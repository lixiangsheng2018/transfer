#include <stdio.h>
#include <stdlib.h>
#include "Sender.h"

#define FLOAT_OPERATE_SUPPORT   1

static const int StepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};

const int IndexTable[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

#define RANG_CHECK(_v,min,max) if((_v) < (min)) (_v) = (min);if ((_v) > (max)) (_v) = (max);

static int PreIndex = 0;
int IMA_ADPCM_BlockEncode(short * indata, unsigned char * outdata, int len){

    int PreSampleVal;
    unsigned int OddEven;
    int sign;           /* Current adpcm sign bit */
    unsigned int Delta; /* Current adpcm output value */
    int diff;           /* Difference between val and valprev */
    unsigned int StepVal;  /* Stepsize */
    unsigned int vpdiff;/* Current change to PreSampleVal */
    int Index;          /* Current step change index */
    unsigned int outputbuffer = 0;/* place to keep previous 4-bit value */
    int Count = 0;      /* the number of bytes encoded */


    PreSampleVal = *((short *)(outdata)) = *indata;
    *((char *)(outdata + 2)) = Index = PreIndex;
    *((char *)(outdata + 3)) = 0x00;
    outdata += 4;
    indata++;
    len--;
    Count = 4;

    OddEven = 1;

    while (len-- > 0 ) {
        diff = *indata - PreSampleVal;
        indata++;
        StepVal = StepTable[Index];
        if(diff < 0){
            sign = 8;
            diff = (-diff);
        } else {
            sign = 0;
        }


#if FLOAT_OPERATE_SUPPORT
        Delta = 0;
        vpdiff = (StepVal >> 3);

        if ( diff >= StepVal ) {
            Delta = 4;
            diff -= StepVal;
            vpdiff += StepVal;
        }
        StepVal >>= 1;
        if ( diff >= StepVal  ) {
            Delta |= 2;
            diff -= StepVal;
            vpdiff += StepVal;
        }
        StepVal >>= 1;
        if ( diff >= StepVal ) {
            Delta |= 1;
            vpdiff += StepVal;
        }
#else
        Delta = diff*4.0/StepVal;
        if (Delta > 7) {
            Delta = 7;
        }
        RANG_CHECK(Delta,0,7);
        vpdiff = (Delta + 0.5) * StepVal / 4.0;
#endif

        if ( sign != 0 ) {
            PreSampleVal -= vpdiff;
        } else {
            PreSampleVal += vpdiff;
        }
        RANG_CHECK(PreSampleVal,-32768,32767);
        Delta |= sign;
        Index += IndexTable[Delta];
        RANG_CHECK(Index,0,88);

        if ( OddEven != 0 ) {
            outputbuffer = (Delta);
        } else {
            *outdata++ = (char)((Delta << 4) | outputbuffer);
            Count++;
        }
        OddEven = !OddEven;
    }

    /* Output last step, if needed */
    if ( OddEven == 0 )
    {
        *outdata++ = (char)outputbuffer;
        Count++;
    }
    
    PreIndex = Index;
    return Count;
}

void IMA_ADPCM_BlockDecode(unsigned char *indata,signed short *outdata, int len){

    unsigned int sign;
    unsigned int delta;
    unsigned int step;
    int SampleVal;
    unsigned int vpdiff;
    int Index;
    int OddEven;

    SampleVal = *outdata = *((short *)(indata));
    outdata++;
    Index = indata[2];
    indata += 4;
    len--;

    OddEven = 1;


    while ( len-- > 0 ) {
        step = StepTable[Index];

        if ( OddEven != 0 ) {
            delta = *indata & 0xf;
        } else {
            delta = (*indata >> 4);
            indata++;
        }
        OddEven = !OddEven;

        Index += IndexTable[delta];
        RANG_CHECK(Index,0,88);

        sign = delta & 8;
        delta = delta & 7;

#if FLOAT_OPERATE_SUPPORT
        vpdiff = step >> 3;
        if ( (delta & 4) != 0 ) vpdiff += step;
        if ( (delta & 2) != 0 ) vpdiff += step>>1;
        if ( (delta & 1) != 0 ) vpdiff += step>>2;

#else
        vpdiff = (delta+0.5)*step/4;
#endif
        if ( sign != 0 ){
            SampleVal -= vpdiff;
        }else{
            SampleVal += vpdiff;
        }
        RANG_CHECK(SampleVal,-32768,32767);
        *outdata++ = (short)SampleVal;
    }
    return;
}


