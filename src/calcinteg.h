#include "calcbk.h"
#include "derinterpquad.h"

double calcinteg(int trg_or_quad, int i, int j, std::vector<Point<2> > coor1)
{
    // calcule l'intégrale du produit scalaire entre les grad de psi_i et psi_j, dans l'élément de coordonnées coor
    // trg_or_quad == 0 means the element is a triangle, anything else means it's a quadrilateral

    double Bk_tBk[2][2] = {{0,0},{0,0}};
    double S;
    if (trg_or_quad == 0)
    {
        // we have to calculate the inverted and transposed matrix of the transformation from the reference triangle to the considered element
        calcbk(trg_or_quad, 0,0, coor1, Bk_tBk);
        //std::cout << "calc bk = " << Bk_tBk[0][0] << " " << Bk_tBk[0][1] << "\n "<< Bk_tBk[1][0] << " " << Bk_tBk[1][1]<< std::endl;
        double gradinterp[3][2];

        gradinterp[0][0] = -1.0;
        gradinterp[0][1] = -1.0;

        gradinterp[1][0] = 1.0;
        gradinterp[1][1] = 0.0;

        gradinterp[2][0] = 0.0;
        gradinterp[2][1] = 1.0;

        S = jacobian(trg_or_quad, 0.0, 0.0, coor1)* (gradinterp[i][0]*(Bk_tBk[0][0]*gradinterp[j][0] + Bk_tBk[0][1]*gradinterp[j][1])+
                gradinterp[i][1]*(Bk_tBk[1][0]*gradinterp[j][1] + Bk_tBk[1][1]*gradinterp[j][1]));
       // std::cout << "jacobien = " << jacobian(trg_or_quad, 0.0, 0.0, coor1) << ", S = " << S << std::endl;
    }

    else
    {
        S = 0;
        std::vector<double> gradi(2), gradj(2);
        double w[5], x[5];
        x[0] = -0.906179845938664;
        x[1] = -0.538469310105683;
        x[2] = 0.0;
        x[3] = 0.538469310105683;
        x[4] = 0.906179845938664;

        w[0] = 0.236926885056189;
        w[1] = 0.478628670499365;
        w[2] = 0.568888889888889;
        w[3] = 0.478628670499365;
        w[4] = 0.236926885056189;

        for (int ii=0; ii<5; ii++) {
            for (int jj = 0;  jj< 5; jj++) {
                calcbk(trg_or_quad, x[ii], x[jj], coor1, Bk_tBk);
                derinterpquad(ii, x[ii], x[jj], gradi);
                derinterpquad(jj, x[ii], x[jj], gradj);
                S+= w[ii]*w[jj]*jacobian(trg_or_quad, x[ii], x[jj], coor1)*(  gradi[0]*(Bk_tBk[0][0]*gradj[0] + Bk_tBk[0][1]*gradj[1])+
                        gradi[1]*(Bk_tBk[1][0]*gradj[0] + Bk_tBk[1][1]*gradj[1])   );
            }
        }

    }
    return S;
}
