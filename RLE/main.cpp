#include <cstdint>
#include <array>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "RLE.h"
#include "RLEex.h"

namespace fs = std::filesystem;

using namespace std;

template < typename T > struct as_bits
{
    static_assert( std::is_trivially_copyable<T>::value, "T must be a TriviallyCopyable type" );
    static_assert( std::is_default_constructible<T>::value, "T must be a DefaultConstructible type" );

    as_bits( const T& v = {} ) : value( v ) {}
    operator T() const { return value; }
    T value;

    friend std::istream& operator>> ( std::istream& stm, as_bits<T>& bits )
    {
        return stm.read( reinterpret_cast<char*>( std::addressof( bits.value ) ), sizeof( bits.value ) );
    }

    friend std::ostream& operator<< ( std::ostream& stm, const as_bits<T>& bits )
    {
        return stm.write( reinterpret_cast<const char*>( std::addressof( bits.value ) ), sizeof( bits.value ) );
    }
};

template < typename T >
using binary_istream_iterator = std::istream_iterator< as_bits<T> >;

template < typename T >
using binary_ostream_iterator = std::ostream_iterator< as_bits<T> >;

int main( int ac, char* av[] )
{
     try
     {
         if( ac != 4 )
         {
             if( ac == 2 && av[1] == "-h" )
             {
                 std::cout << "RLE archiving: -p|-u|-pb|-p input_file output_file\n";
                 std::cout << "-p\tarchiving using RLE-ex algorithm\n";
                 std::cout << "-u\tUnarchiving by RLE-ex algorithm\n";
                 std::cout << "-pb\tarchiving using RLE algorithm\n";
                 std::cout << "-ub\tUnarchiving by RLE algorithm\n";
             }
             else
                throw std::runtime_error( "incorec input param: -p|-u|-pb|-p input_file output_file" );
         }

        std::string regime = av[1];
        fs::path input_file = av[2];
        fs::path output_file = av[3];

        std::ifstream ifs( input_file, std::ios::binary );
        if( !ifs.is_open() )
            throw std::runtime_error( "not open file: " + input_file.string() );
        std::ofstream ofs( output_file, std::ios::binary );
        if( !ofs.is_open() )
            throw std::runtime_error( "not open file: " + output_file.string() );

        auto first = binary_istream_iterator<uint8_t>( ifs );
        auto last = binary_istream_iterator<uint8_t>();
        auto dest = std::ostream_iterator<uint8_t>( ofs );

        if( regime == "-pb" )
        {
            RLE::pack( first, last, dest );
        }
        else if( regime == "-ub" )
        {
            RLE::unpack( first, last, dest );
        }
        else if( regime == "-p" )
        {
            RLE_Ex::pack( first, last, dest );
        }
        else if( regime == "-u" )
        {
            RLE_Ex::unpack( first, last, dest );
        }
        else
        {
            throw std::runtime_error( "incorect param, read -h" );
        }
    }
    catch( const std::exception& err )
    {
        std::cerr << err.what() << '\n';
        system( "pause" );
        return 1;
    }

    return 0;
}
