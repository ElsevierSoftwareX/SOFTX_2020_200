#include "quadboundary.h"
#include "trgboundary.h"
#include "deal.II/base/point.h"
#include <vector>

#include "ib_node_status.h"
#ifndef NOUVTRIANGLES_H
#define NOUVTRIANGLES_H


using namespace dealii;

// NOTE BB : La fonction nouvtriangle devrait renvoyer le statut de l'élément : Inside, Fluid, Cut_1Tri, Cut_Quad, Cut_3Tri
// NOTE BB2 : Il faudrait aussi avoir la reference aux DOF originaux...

void nouvtriangles(std::vector<int> &corresp, std::vector<node_status> &No_pts_solid, std::vector<Point<2> > &num_elem, std::vector<Point<2> > &decomp_elem, int* nb_poly, std::vector<Point<2> > coor_elem1, std::vector<double> val_f1)
{
    /* *** decomp_elem is a vector in which we shall store the coordinates of the sub_elements created with this function

     *** nb_poly will allow us to indicate the number of sub_elements created in decomp_elem and to specify if we return triangles or a quadrilateral
     nb_ poly is pretty simple :
        - positive if the sub_elements are triangles, then it indicates the number created
        - 0 if the element considered is totally in the fluid or in the solid
        - -1 if we create a quadrilateral

     *** corresp : returns the equivalence between the local numerotation in the sub-elements and the numerotation in the initial element, its length is 9 so that we don,t have to create a dynamic array
     so for ex. if we create only one triangle, we shall fill only the 3 first elements of corresp and then we will put -1 to all the others elements of corresp
     Example :

     2-5--3     Let this rectangle be the element studied. For this example, the vertices 0, 1 and 3 are in the solid, 4 and 5 are the points of intersection between the sides of the rectangle and the boundary
     |/   |
     4    |     The vertices of the sub-element created would be 2, 4 and 5. Thus, corresp would be {2, 4, 5, -1, -1,  ..., -1}
     |    |     Here nb_poly = 1. Decom_elem gives the coordinates of 2, 4 and 5 (in this order !).
     0----1

     *** No_pts_solid allows you to know if the i-th vertex is in the fluid or in the solid.

     *** num_elem[i] gives the coordinates of the point associated to the number i in the numerotation of the element. In this example, num_elem[0] would be the coordinates of the point 0, and thus would be equal to coor_elem1[0].

     *** You also have to give :
     - coor_elem1 : the coordinates of each vertex of the element considered
     - val_f1 : the value of the distance function for each vertex of the element

    IMPORTANT :
     This code is made to work with the following numerotation for the vertices of the element

          1----0
          |    |
          2----3

     However the numerotation used in deal.II is as follows

          2----3
          |    |
          0----1

     Thus we shall make a local numerotation for the vertices, and create a local vector for the distance function matching with it.
     FOR THAT WE JUST HAVE TO USE THE VECTOR "vec_change_coor"

*/

    const int npt=4;

    std::vector<Point<2> > coor_elem(4);
    std::vector<double> val_f(4);

    //////int inv_vec_change_coor[4] {2, 3, 1, 0}; // to change coor from ref coor to deal.II coor
    const int vec_change_coor[4] {3, 2, 0, 1}; // to change coor from deal.II coor to ref coor


    for (int jj = 0; jj < 4; ++jj) { // changing the coordinates
        coor_elem[jj]  = coor_elem1[vec_change_coor[jj]];
        val_f[jj] = val_f1[vec_change_coor[jj]];
        //std::cout << coor_elem[jj] << std::endl;
    }

    // Calculate size of element to establish tolerance
    double size=0;
    for (int i = 1 ; i < npt ; ++i)
    {
      size = std::max(size,coor_elem[0].distance(coor_elem[i]));
    }

    double accuracy = 1e-3*size;

    // We will start by finding if there are any changes in the sign of the distance function among the the summits

    std::vector<int> a(4);
    int sum_a =0;
    for (int i = 0; i < 4 ; i++)
    {
        a[i]=1;
        No_pts_solid[vec_change_coor[i]] = fluid;
        if (val_f[i]<0) {a[i]=-1; No_pts_solid[vec_change_coor[i]] = solid;}

        else if ((val_f[i]<accuracy) && (val_f[i]>-accuracy))
        {
            a[i]=-1;
            val_f[i] = -accuracy;
            No_pts_solid[vec_change_coor[i]] = solid;
        }
        sum_a += a[i];
    }

    // sum_a indicates if there are more summits in the fluid than in the solid (it is then strictly positive)

    if (sum_a==0) // there are as many summits in the fluid as in the solid
    {

        for (int i = 0; i < 4 ; i++)
        {

            if (a[i]>0)
            {
                if ((i==0) && (a[3]>0)) // this case exists just to have always the same order in the summits of the sub-elements
                {
                    quadboundary(4, decomp_elem, coor_elem, val_f);

                    corresp = {4, 5, vec_change_coor[3], vec_change_coor[0],-1,-1,-1,-1,-1};
                    num_elem = {coor_elem1[0], coor_elem1[1], coor_elem1[2], coor_elem1[3], decomp_elem[0], decomp_elem[1]};

                    *nb_poly = -1;
                    break;
                }

                else
                {
                    quadboundary(i+1, decomp_elem, coor_elem, val_f);

                    corresp = {4, 5, vec_change_coor[(i+1) % 4], vec_change_coor[i],-1,-1,-1,-1,-1};
                    num_elem = {coor_elem1[0], coor_elem1[1], coor_elem1[2], coor_elem1[3], decomp_elem[0], decomp_elem[1]};

                    *nb_poly = -1;
                    break;
                }
            }
        }
    }


    else if (sum_a<0 && sum_a>-3) // case where there is one or less summit in the fluid part
    {
            int b=0;
            for (int i = 0; i < 4; ++i)
            {
                if (a[i]==1){b=i; break;}
            }

            std::vector<Point<2> >      boundary_pts(2);
            trgboundary(b, boundary_pts, coor_elem, val_f);

            decomp_elem[0]=coor_elem[b];
            decomp_elem[1]=boundary_pts[0];
            decomp_elem[2]=boundary_pts[1];


//            for (int i = 0; i < 3; ++i) {
//                if (i%3==0) std::cout << "\n" << std::endl;
//                std::cout << decomp_elem[i] << std::endl;

//            }
//            std::cout << "\n" << std::endl;



            num_elem = {coor_elem1[0], coor_elem1[1], coor_elem1[2], coor_elem1[3], boundary_pts[0], boundary_pts[1]};
            corresp = {vec_change_coor[b], 4, 5, -1,-1,-1,-1,-1};

            *nb_poly = 1;   
    }


    else if (sum_a>=3 || sum_a<=-3){*nb_poly = 0;}


    else if (sum_a>0 && sum_a<3) // there is up to one summit in the solid zone
    {
        int b=0;
        for (int i = 0; i < 4; ++i)
        {
            if (a[i]==-1){b=i; break;}
        }

        if (sum_a==2)
        {

                std::vector<Point<2> >      boundary_pts(2);
                trgboundary(b, boundary_pts, coor_elem, val_f);
                decomp_elem [0] = coor_elem[(b+1)%4];
                decomp_elem [1] = coor_elem[(b+2)%4];
                decomp_elem [2] = boundary_pts[0];

                decomp_elem [3] = coor_elem[(b+2)%4];
                decomp_elem [4] = boundary_pts[1];
                decomp_elem [5] = boundary_pts[0];

                decomp_elem [6] = coor_elem[(b+3)%4];
                decomp_elem [7] = boundary_pts[1];
                decomp_elem [8] = coor_elem[(b+2)%4];


//                for (int i = 0; i < 9; ++i) {
//                    if (i%3==0) std::cout << "\n" << std::endl;
//                    std::cout << decomp_elem[i] << std::endl;

//                }
//                std::cout << "\n" << std::endl;


                corresp = {vec_change_coor[(b+1)%4], vec_change_coor[(b+2)%4], 5, vec_change_coor[(b+2)%4], 4, 5, vec_change_coor[(b+3)%4],4 , vec_change_coor[(b+2)%4] };
                num_elem = {coor_elem1[0], coor_elem1[1], coor_elem1[2], coor_elem1[3], boundary_pts[1], boundary_pts[0]}; // boundary points are sorte differently because of the way the function trgboundary works

                *nb_poly = 3;
        }
    }
}
#endif //NOUVTRIANGLES_H