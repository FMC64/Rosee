#include <stdio.h>

// stuff this in a header somewhere
inline int type_id_seq = 0;
template< typename T > inline const int type_id = type_id_seq++;

int main() {
        printf( "%d\n", type_id< int > );
        printf( "%d\n", type_id< float > );
        printf( "%d\n", type_id< int > );
        printf( "%d\n", type_id< int > );
        printf( "%d\n", type_id< int > );
        printf( "%d\n", type_id< size_t > );
        return 0;
}