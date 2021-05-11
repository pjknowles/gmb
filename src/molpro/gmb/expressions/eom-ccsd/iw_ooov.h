#ifndef GMB_EOM_CCSD_IW_OOOV_H
#define GMB_EOM_CCSD_IW_OOOV_H

#include <libtensor/libtensor.h>
#include "../../container.h"

/**
 * @brief W_ooov intermediate
 * 
 */
container<4,double> eom_ccsd_iw_ooov(
    container<2, double> &t1,       ///< CCSD T1
    container<4, double> &t2,       ///< CCSD T2 intermediate
    container<4, double> &tau,      ///< tau intermediate
    container<2, double> &if_ov,    ///< fock matrix - ov block
    container<4, double> &iw_oooo,  ///> anti-symmetrized integral <ij||kl>
    container<4, double> &i_ooov,   ///> anti-symmetrized integral <ij||ka>
    container<4, double> &i_oovv,   ///> anti-symmetrized integral <ij||ab>
    container<4, double> &i_ovov,   ///> anti-symmetrized integral <ia||jb>
    container<4, double> &i_ovvv    ///> anti-symmetrized integral <ia||jb>

) {

    container<4, double> iw_ooov(i_ooov.get_space());

    libtensor::letter i,j,k,m,a,e,f;

    iw_ooov(i|j|k|a) = i_ooov(i|j|k|a)
                     - contract(m, t1(m|a), iw_oooo(i|j|k|m))
                     + 0.5 * contract(e|f, tau(i|j|e|f), i_ovvv(k|a|e|f))
                     - asymm(i, j,
                        contract(e, t1(i|e), 
                            i_ovov(k|a|j|e)
                            - contract(m|f, t2(j|m|a|f), i_oovv(k|m|e|f)))
                       + contract(m|e, t2(i|m|a|e), i_ooov(k|m|j|e)))
                     + contract(e, t2(i|j|a|e), 
                         contract(m|f, t1(m|f), i_oovv(k|m|f|e)))
                     - contract(e, t2(i|j|a|e), if_ov(k|e));

    return iw_ooov;

}

#endif // GMB_EOM_CCSD_IW_OOOV_H
