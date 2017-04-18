/// Implementation of the Reed-Solomon algo in O(N*log(N)) using Number-Theoretical Transform in GF(P)
#include <iostream>
#include <algorithm>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <cassert>
#include <algorithm>
#include <utility>
#include <functional>
#include <vector>
#include <memory>

#include "wall_clock_timer.h"
#include "GF(p).cpp"
#include "NTT.cpp"


// Benchmark encoding using the ReedSolomon algo
template <typename T, T P>
void EncodeReedSolomon (size_t N, size_t SIZE)
{
    T *data0 = new T[N*SIZE];
    for (size_t i=0; i<N*SIZE; i++)
        data0[i] = i%P;

    T **data = new T* [N];      // pointers to blocks
    for (size_t i=0; i<N; i++)
        data[i] = data0 + i*SIZE;

    char title[99];
    sprintf (title, "ReedSolomon encoding <2^%d,%d>", 20/*lb(N)*/, int(SIZE*sizeof(T)));

    time_it (2*N*SIZE*sizeof(T), title, [&]
    {
        // 1. iNTT: polynom interpolation. We find coefficients of order-N polynom describing the source data
        MFA_NTT <T,P> (N, SIZE, data, false);

        // Now we can evaluate the polynom in 2*N points.
        // Points with even index will contain the source data,
        // while points with odd indexes may be used as ECC data.
        // But more efficient approach is to compute only odd-indexed points.
        // This is accomplished by the following algo:

        // 2. Multiply the polynom coefficents by root(2*N)**i
        T root = GF_Root<T,P>(N);
        T root_i = root;                            // root**1
        #pragma omp parallel for
        for (ptrdiff_t i=1; i<N; i++) {
            T* __restrict__ block = data[i];
            for (size_t k=0; k<SIZE; k++) {         // cycle over SIZE elements of the single block
                block[k] = GF_Mul<T,P> (block[k], root_i);
            }
            root_i = GF_Mul<T,P> (root_i, root);    // root**i for the next i
        }

        // 3. NTT: polynom evaluation. This evaluates the modified polynom at root(N)**i points,
        // that is equivalent to evaluating the original polynom at root(2*N)**(2*i+1) points.
        MFA_NTT <T,P> (N, SIZE, data, true);

        // Further optimization: in order to compute only even-indexed points,
        // it's enough to compute order-N/2 NTT of data[i]+data[i+N/2]. And so on...
    });
}

int main (int argc, char **argv)
{
    size_t N = 1<<20;   // NTT order
    size_t SIZE = 512;  // Block size, in bytes
                        // 512 MB total

    if (argc>=2)  N = 1<<atoi(argv[1]);
    if (argc>=3)  SIZE = atoi(argv[2]);

    EncodeReedSolomon<uint32_t,0xFFF00001> (N,SIZE/sizeof(uint32_t));
}
