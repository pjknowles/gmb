#ifndef GMB_EOM_CCSD_IF_VV_H
#define GMB_EOM_CCSD_IF_VV_H

#include <libtensor/libtensor.h>
#include "../../container.h"

/**
 * @brief F_vv intermediate
 * 
 */
container<2,double> eom_ccsd_if_vv(
    container<2, double> &t1,         ///< CCSD T1
    container<4, double> &t2,         ///< CCSD T2
    container<2, double> &f_ov,       ///< fock matrix - ov block
    container<2, double> &f_vv,       ///< fock matrix - vv block
    container<4, double> &i_oovv,     ///> anti-symmetrized integral <ij||ab>
    container<4, double> &i_ovvv      ///> anti-symmetrized integral <ia||bc>
) {

    container<2, double> if_vv(f_vv.get_space());

    libtensor::letter i,j,m,n,a,b,e,f;

    if_vv(b|e) = f_vv(b|e)
               - contract(m, t1(m|b), f_ov(m|e))
               - 0.5 * contract(f|m|n, t2(m|n|b|f), i_oovv(m|n|e|f))
               - contract(f|m|n, t1(m|b)*t1(n|f), i_oovv(m|n|e|f))
               + contract(f|m, t1(m|f), i_ovvv(m|b|f|e));
    return if_vv;

}

#endif //GMB_EOM_CCSD_IF_VV_H
