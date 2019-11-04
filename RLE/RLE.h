#pragma once

#include <cstdint>

#include <stdexcept>
#include <vector>
#include <limits>

using Bytes = std::vector<uint8_t>;

class RLE
{
public:
    static Bytes pack( const Bytes& str )
    {
        Bytes out;
        if( str.empty() )
            return out;
        out.reserve( str.size() );

        auto prevC = str[0];
        uint8_t s = 0;
        for( size_t i = 1; i < str.size(); ++i )
        {
            auto c = str[i];
            if( c != prevC || ( s == 255 ) )
            {
                out.push_back( Bytes::value_type( s ) );
                out.push_back( prevC );
                s = 0;
                prevC = c;
            }
            else
            {
                ++s;
            }

        }
        out.push_back( Bytes::value_type( s ) );
        out.push_back( prevC );

        return out;
    }

    static Bytes unpack( const Bytes& str )
    {
        if( str.size() % 2 )
            throw std::runtime_error( "incorect RLE pack" );
        Bytes out;

        for( size_t i = 0; i < str.size(); i += 2 )
        {
            size_t count = static_cast<size_t>( str[i] ) + 1;
            auto c = str[i + 1];
            out.insert( out.end(), count, c );
        }

        return out;
    }

    template< class FwdIt1, class FwdIt2 >
    static void pack( FwdIt1 first, FwdIt1 last, FwdIt2 dest )
    {
        using value_type = uint8_t;

        if( first == last )
            return;

        value_type prev = *first;
        ++first;
        value_type size = 0;
        for( ; first != last; ++first )
        {
            if( *first != prev || ( size == ( std::numeric_limits<value_type>::max )( ) ) )
            {
                *dest = size;
                ++dest;
                *dest = prev;
                prev = *first;
                size = 0;
            }
            else
            {
                ++size;
            }
        }
        *dest = size;
        ++dest;
        *dest = prev;
    }

    template< class FwdIt1, class FwdIt2 >
    static void unpack( FwdIt1 first, FwdIt1 last, FwdIt2 dest )
    {
        using value_type = uint8_t;

        if( first == last )
            return;

        for( ; first != last; ++first )
        {
            auto s = *first + value_type( 1 );
            ++first;
            if( first == last )
                throw std::runtime_error( "incorect RLE pack" );
            std::fill_n( dest, s, *first );
        }
    }
};
