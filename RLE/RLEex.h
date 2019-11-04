#pragma once

#include "RLE.h"

class RLE_Ex
{
    static const size_t maxPackBytes = 127; // считаем с 0
public:
    static Bytes pack( const Bytes& str )
    {
        // первым проходом сжимаем RLE
        Bytes out = RLE::pack( str );

        // вторым, сжимаем одиночные сжатия в одну последовательность
        size_t r = 0;
        size_t pos = 0;
        auto writeRaw = [&]( size_t i )
        {
            out[pos] = Bytes::value_type( r + maxPackBytes );
            ++pos;

            for( size_t j = 0; j < r; ++j, ++pos )
            {
                out[pos] = out[i + 1 - r * 2 + j * 2];
            }
        };

        for( size_t i = 0; i < out.size(); i += 2 )
        {
            if( out[i] == 0 )
            {
                if( r == 128 )
                {
                    writeRaw( i );
                    r = 0;
                }
                r++;
            }
            else
            {
                if( r > 0 )
                {
                    if( r == 1 )
                    {
                        out[pos++] = out[i - 2];
                        out[pos++] = out[i - 1];
                    }
                    else
                    {
                        writeRaw( i );
                    }
                    r = 0;
                }
                out[pos++] = out[i];
                out[pos++] = out[i + 1];
            }
        }
        if( r > 0 )
        {
            writeRaw( out.size() );
        }
        out.resize( pos );
        return out;
    }

    static Bytes unpack( const Bytes& str )
    {
        Bytes out;

        size_t i = 0;
        while( i < str.size() )
        {
            size_t count = static_cast<size_t>( str[i] );
            ++i;
            if( count > maxPackBytes )
            {
                count -= maxPackBytes;
                if( i + count > str.size() )
                    throw std::runtime_error( "incorect file" );
                std::copy( str.begin() + i, str.begin() + i + count, std::back_inserter( out ) );
                i += count;
            }
            else
            {
                if( i == str.size() )
                    throw std::runtime_error( "incorect file" );
                out.insert( out.end(), count + 1, str[i++] );
            }
        }

        return out;
    }

    template< class FwdIt1, class FwdIt2 >
    static void pack( FwdIt1 first, FwdIt1 last, FwdIt2 dest )
    {
        using value_type = uint8_t;

        if( first == last )
            return;

        value_type prev = *( first );
        ++first;
        size_t limitRaw = 255 - maxPackBytes;
        size_t limitZip = maxPackBytes + 1;
        std::array< value_type, 255 > buffer;
        size_t sizeRaw = 0;
        size_t sizeZip = 1;

        // записать последовательность повторяющихся символов
        auto writeZIP = [&]( value_type c, size_t count )
        {
            // вычитаем единицу чтобы расширить диапозон на 1, задействов 0
            *dest = count - 1;
            ++dest;
            *dest = c;
        };
        // записать последовательность символов
        auto writeRAW = [&]( size_t count )
        {
            *dest = count + maxPackBytes;
            ++dest;
            std::copy( std::begin( buffer ), std::begin( buffer ) + count, dest );
        };
        /*
            По состоянию предыдущего(prev) и текущего(curr) мы определяем что сейчас за последовательность.
            Если curr == prev, то началась ZipSq(aaaaa) последовательность и необходимо записать RawSq(ababababab),
            иначе RawSq и необходимо записать ZipSq.
        */

        for( ; first != last; ++first )
        {
            auto curr = *first;
            if( prev == curr )
            {
                // Есть RAW данные
                if( sizeRaw > 0 )
                {
                    // sizeRaw [1-...], записываем
                    writeRAW( sizeRaw );
                    sizeRaw = 0;
                    sizeZip = 1;
                }

                // переполнилось, записываем, сбрасываем подсчет
                // записывается с учетом текущего prev,
                // если бы сделали после прибавки 1, то записали curr
                if( sizeZip == limitZip )
                {
                    writeZIP( prev, sizeZip );
                    sizeZip = 0;
                }
                ++sizeZip;
            }
            else
            {
                // есть посторяющиеся символы
                if( sizeZip > 0 )
                {
                    // только один, мы запишем в буфер, т.к. это начало последовательности неповторяющихся символов
                    if( sizeZip == 1 )
                    {
                        buffer[sizeRaw] = prev;
                        sizeRaw += 1;
                        sizeZip = 0;
                    }
                    else
                    {
                        // sizeZip [2...]
                        writeZIP( prev, sizeZip );
                        sizeZip = 1;
                    }

                }
                else
                {
                    buffer[sizeRaw] = prev;
                    sizeRaw += 1;
                    // переполнилось, записываем, сбрасываем подсчет
                    if( sizeRaw == limitRaw )
                    {
                        writeRAW( sizeRaw );
                        sizeRaw = 0;
                        // учитываем prevC, мб это начало повторяющейся последовательности
                        sizeZip = 1;
                    }
                }
                prev = curr;
            }
        }
        // Дозаписываем
        if( sizeZip > 0 )
        {
            writeZIP( prev, sizeZip );
        }
        else if( sizeRaw > 0 )
        {
            if( sizeRaw == 1 )
            {
                writeZIP( prev, 1 );
            }
            else
            {
                // prev символ не учтен, т.к. последняя была RawSq, то он является её частью
                buffer[sizeRaw] = prev;
                sizeRaw += 1;
                writeRAW( sizeRaw );
            }
        }
    }


    template< class FwdIt1, class FwdIt2 >
    static void unpack( FwdIt1 first, FwdIt1 last, FwdIt2 dest )
    {
        using value_type = uint8_t;

        if( first == last )
            return;

        for( ; first != last; )
        {
            size_t size = size_t( *first );
            ++first;
            // это RawSq
            if( size > maxPackBytes )
            {
                // вычисляем число символов
                size -= maxPackBytes;
                // copy
                for( size_t i = 0; i < size; ++i, ++first )
                {
                    if( first == last )
                        throw std::runtime_error( "incorect file RLE" );
                    *dest = *first;
                }
            }
            else
            {
                size += value_type( 1 );
                if( first == last )
                    throw std::runtime_error( "incorect file RLE" );
                std::fill_n( dest, size, *first );
                ++first;
            }
        }
    }
};
