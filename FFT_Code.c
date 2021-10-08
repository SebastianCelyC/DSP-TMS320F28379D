/////////////////////////////////////////////////////////////////////
///                         LABORATORIO FFT                       ///
///                  MARIA SAA - SEBASTIAN CELY                   ///
/////////////////////////////////////////////////////////////////////

#include "F28x_Project.h"
#include "math.h"

#define N 512
#define N_2  N >> 1 //es N/2
#define Arg 2*M_PI/N

void Mul_RI(float *P1, float *P2);
void FFTx(void);
void Suma_Real(void);
void Suma_Ima(void);
void Mul_W(void);
void Mag_Ang(void);

Uint16 BIT_INV(Uint16 Addr);
Uint16 TMP;
Uint16 Buff_I[N];                          //Buffer Inverso
Uint16 Offset = 1;

float x[N];
float Y[N][2];
float W[N_2][2];
float Mag[N];
float Pha[N];

void main(void)
{
    int i;
    InitSysCtrl();

    // Llenar buffer con bit inverso
    for (i = 0;  i < N; i++)
    {
        Buff_I[i] =  BIT_INV(i);
    }

    // Llenar  W
    for (i = 0;  i < N_2; i++)
    {
      W[i][0] = cos(Arg*i);
      W[i][1] = -sin(Arg*i);
    }

    // Llenar buffer de entrada con valores de una rampa
    for (i = 0;  i < N; i++)
    {
       x[i] = (float)i*1.0;
       Y[i][0] = 0;
       Y[i][1] = 0;
    }

    FFTx();
    Mag_Ang();

    while(1){};
}

void Mag_Ang()
{
    Uint16 i;
    float R = 0 , Im = 0;
    for (i = 0; i < N; i++)
    {
        R = Y[i][0];
        Im = Y[i][1];

        Y[i][0] = sqrt(pow(R,2) + pow(Im,2))/N;      ///Magnitud

        if(i != 0)
        {
            Y[i][0] = Y[i][0] * 2;
        }

        Mag[i] = Y[i][0];

        Y[i][1] = 180/M_PI*atan(Im/R);              //Fase

        if(Im >= 0 & R >= 0)
        {
            Y[i][1] += 0;
        }

        else if(Im >= 0 & R < 0)
        {
            Y[i][1] = 180 + Y[i][1];
        }

        else if(Im < 0 & R < 0)
        {
            Y[i][1] = -180 + Y[i][1];
        }
        else
        {
            Y[i][1] += 0;
        }
    }

}

void FFTx()
{
    Suma_Real();
    do
    {
        Offset <<= 1;
        Mul_W();
        Suma_Ima();
    }while(Offset < N_2);
}

void Suma_Real(void)
{
    Uint16 i;

    for (i = 0; i < N; i += 2*Offset)
    {
        Y[i][0] = x[Buff_I[i]] + x[Buff_I[i + Offset]];
        Y[i + Offset][0] = x[Buff_I[i]] - x[Buff_I[i + Offset]];
    }
}

void Mul_RI(float *P1, float *P2)
{
    float tmp;
    tmp = *(P1) * *(P2) - *(P1+1) * *(P2+1);
    *(P1+1) = *(P1) * *(P2+1) + *(P2) * *(P1+1);
    *(P1) = tmp;
}

void Suma_Ima(void)
{
    Uint16 i, j;
    float Temp_A, Temp_B;

    for (i = 0; i < N; i += 2*Offset)
    {
        for ( j= 0; j < Offset; j++)
        {
            Temp_A = Y[i + j][0] + Y[i + j + Offset][0];
            Temp_B = Y[i + j][1] + Y[i + j + Offset][1];
            Y[i + j + Offset][0] = Y[i + j][0] - Y[i + j + Offset][0];
            Y[i + j + Offset][1] = Y[i + j][1] - Y[i + j + Offset][1];
            Y[i + j][0] = Temp_A;
            Y[i + j][1] = Temp_B;
        }
    }
}

void  Mul_W(void)
{
    Uint16 i, j, R, Div;

    Div = N/(2*Offset);
    for (i = Offset; i < N; i += 2*Offset)
    {
        for (j = 1; j < Offset; j++)
        {
            R = j*Div;
            Mul_RI(&Y[i + j][0],&W[R][0]);
        }
    }
}

Uint16 BIT_INV(Uint16 i)
{
    Uint16 M1, M2, Result;

    Result = 0;
    for(M1 = 1, M2 = N>>1; M1 < N; M1 <<= 1, M2 >>= 1)
    {
        if(i & M1) Result |= M2;
    }
    return(Result);
}
