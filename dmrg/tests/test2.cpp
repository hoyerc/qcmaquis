/*****************************************************************************
 *
 * ALPS MPS DMRG Project
 *
 * Copyright (C) 2014 Institute for Theoretical Physics, ETH Zurich
 *               2011-2013 by Michele Dolfi <dolfim@phys.ethz.ch>
 *               2019 by Leon Freitag <lefreita@ethz.ch>
 *
 * This software is part of the ALPS Applications, published under the ALPS
 * Application License; you can use, redistribute it and/or modify it under
 * the terms of the license, either version 1 or (at your option) any later
 * version.
 *
 * You should have received a copy of the ALPS Application License along with
 * the ALPS Applications; see the file LICENSE.txt. If not, the license is also
 * available from http://alps.comp-phys.org/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/

#define BOOST_TEST_MAIN

#include <boost/test/included/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "utils/io.hpp" // has to be first include because of impi
#include <iostream>

#include "dmrg/models/chem/integral_interface.h"
#include "dmrg/utils/DmrgParameters.h"

#include "maquis_dmrg.h"

// Test 2: LiH with d=1 Angstrom, singlet, cc-pVDZ basis set, CAS(2,4), integrals generated by MOLCAS
BOOST_AUTO_TEST_CASE( Test2 )
{

    DmrgParameters p;

    // const std::string integrals("   0.597263715971             1     1     1     1 \n "
    //                             "   0.106899493032E-01         1     1     2     1 \n "
    //                             "   0.215106203079E-02         2     1     2     1 \n "
    //                             "   0.310109624236E-02         2     2     2     1 \n "
    //                             "   0.205585394307             1     1     2     2 \n "
    //                             "   0.266391740477             2     2     2     2 \n "
    //                             "  -0.294917730728E-08         1     1     3     1 \n "
    //                             "   0.185509604436E-07         2     1     3     1 \n "
    //                             "  -0.437276233593E-07         2     2     3     1 \n "
    //                             "   0.983873746561E-01         3     1     3     1 \n "
    //                             "  -0.550389961495E-02         3     2     3     1 \n "
    //                             "  -0.148187533022E-07         3     3     3     1 \n "
    //                             "   0.656187599279E-07         1     1     3     2 \n "
    //                             "  -0.686302758417E-08         2     1     3     2 \n "
    //                             "  -0.185120456521E-07         2     2     3     2 \n "
    //                             "   0.329840207584E-02         3     2     3     2 \n "
    //                             "   0.705823689976E-07         3     3     3     2 \n "
    //                             "   0.505228366944             1     1     3     3 \n "
    //                             "   0.402391626267E-02         2     1     3     3 \n "
    //                             "   0.207289323817             2     2     3     3 \n "
    //                             "   0.485199794875             3     3     3     3 \n "
    //                             "  -0.179578099756             1     1     4     1 \n "
    //                             "  -0.968917151241E-02         2     1     4     1 \n "
    //                             "  -0.985834429751E-02         2     2     4     1 \n "
    //                             "   0.166302185468E-07         3     1     4     1 \n "
    //                             "  -0.125974448467E-07         3     2     4     1 \n "
    //                             "  -0.113847869990             3     3     4     1 \n "
    //                             "   0.118352897835             4     1     4     1 \n "
    //                             "   0.102654021605E-01         4     2     4     1 \n "
    //                             "  -0.130590354090E-07         4     3     4     1 \n "
    //                             "  -0.121351408757             4     4     4     1 \n "
    //                             "  -0.350908680238E-01         1     1     4     2 \n "
    //                             "   0.232966449115E-02         2     1     4     2 \n "
    //                             "   0.137817149158E-01         2     2     4     2 \n "
    //                             "   0.112151199425E-07         3     1     4     2 \n "
    //                             "  -0.240005477894E-07         3     2     4     2 \n "
    //                             "  -0.312450484337E-01         3     3     4     2 \n "
    //                             "   0.123070217302E-01         4     2     4     2 \n "
    //                             "  -0.474523186140E-09         4     3     4     2 \n "
    //                             "  -0.249154148625E-01         4     4     4     2 \n "
    //                             "   0.405834174486E-07         1     1     4     3 \n "
    //                             "   0.416033554153E-08         2     1     4     3 \n "
    //                             "  -0.294525852258E-07         2     2     4     3 \n "
    //                             "  -0.444089181829E-02         3     1     4     3 \n "
    //                             "  -0.612942989364E-02         3     2     4     3 \n "
    //                             "   0.350197853722E-07         3     3     4     3 \n "
    //                             "   0.251137992170E-01         4     3     4     3 \n "
    //                             "   0.323896546368E-07         4     4     4     3 \n "
    //                             "   0.450421787348             1     1     4     4 \n "
    //                             "   0.671517333359E-02         2     1     4     4 \n "
    //                             "   0.195606443342             2     2     4     4 \n "
    //                             "   0.185362625807E-08         3     1     4     4 \n "
    //                             "   0.419234994857E-07         3     2     4     4 \n "
    //                             "   0.382638069339             3     3     4     4 \n "
    //                             "   0.370380122890             4     4     4     4 \n "
    //                             "  -0.876082926130             1     1     0     0 \n "
    //                             "   0.928246209082E-02         2     1     0     0 \n "
    //                             "  -0.383002645306             2     2     0     0 \n "
    //                             "   0.110478689971E-07         3     1     0     0 \n "
    //                             "  -0.838075467983E-07         3     2     0     0 \n "
    //                             "  -0.192723200850             3     3     0     0 \n "
    //                             "   0.118275609870             4     1     0     0 \n "
    //                             "   0.539573313879E-01         4     2     0     0 \n "
    //                             "  -0.670886115563E-07         4     3     0     0 \n "
    //                             "  -0.240135259399             4     4     0     0 \n "
    //                             "   -6.71049529388             0     0     0     0 \n ");
    // p.set("integrals", integrals);

    const chem::integral_map<double> integrals  {
                                                { { 1, 1, 1, 1 },   0.597263715971     },
                                                { { 1, 1, 2, 1 },   0.106899493032E-01 },
                                                { { 2, 1, 2, 1 },   0.215106203079E-02 },
                                                { { 2, 2, 2, 1 },   0.310109624236E-02 },
                                                { { 1, 1, 2, 2 },   0.205585394307     },
                                                { { 2, 2, 2, 2 },   0.266391740477     },
                                                { { 1, 1, 3, 1 },  -0.294917730728E-08 },
                                                { { 2, 1, 3, 1 },   0.185509604436E-07 },
                                                { { 2, 2, 3, 1 },  -0.437276233593E-07 },
                                                { { 3, 1, 3, 1 },   0.983873746561E-01 },
                                                { { 3, 2, 3, 1 },  -0.550389961495E-02 },
                                                { { 3, 3, 3, 1 },  -0.148187533022E-07 },
                                                { { 1, 1, 3, 2 },   0.656187599279E-07 },
                                                { { 2, 1, 3, 2 },  -0.686302758417E-08 },
                                                { { 2, 2, 3, 2 },  -0.185120456521E-07 },
                                                { { 3, 2, 3, 2 },   0.329840207584E-02 },
                                                { { 3, 3, 3, 2 },   0.705823689976E-07 },
                                                { { 1, 1, 3, 3 },   0.505228366944     },
                                                { { 2, 1, 3, 3 },   0.402391626267E-02 },
                                                { { 2, 2, 3, 3 },   0.207289323817     },
                                                { { 3, 3, 3, 3 },   0.485199794875     },
                                                { { 1, 1, 4, 1 },  -0.179578099756     },
                                                { { 2, 1, 4, 1 },  -0.968917151241E-02 },
                                                { { 2, 2, 4, 1 },  -0.985834429751E-02 },
                                                { { 3, 1, 4, 1 },   0.166302185468E-07 },
                                                { { 3, 2, 4, 1 },  -0.125974448467E-07 },
                                                { { 3, 3, 4, 1 },  -0.113847869990     },
                                                { { 4, 1, 4, 1 },   0.118352897835     },
                                                { { 4, 2, 4, 1 },   0.102654021605E-01 },
                                                { { 4, 3, 4, 1 },  -0.130590354090E-07 },
                                                { { 4, 4, 4, 1 },  -0.121351408757     },
                                                { { 1, 1, 4, 2 },  -0.350908680238E-01 },
                                                { { 2, 1, 4, 2 },   0.232966449115E-02 },
                                                { { 2, 2, 4, 2 },   0.137817149158E-01 },
                                                { { 3, 1, 4, 2 },   0.112151199425E-07 },
                                                { { 3, 2, 4, 2 },  -0.240005477894E-07 },
                                                { { 3, 3, 4, 2 },  -0.312450484337E-01 },
                                                { { 4, 2, 4, 2 },   0.123070217302E-01 },
                                                { { 4, 3, 4, 2 },  -0.474523186140E-09 },
                                                { { 4, 4, 4, 2 },  -0.249154148625E-01 },
                                                { { 1, 1, 4, 3 },   0.405834174486E-07 },
                                                { { 2, 1, 4, 3 },   0.416033554153E-08 },
                                                { { 2, 2, 4, 3 },  -0.294525852258E-07 },
                                                { { 3, 1, 4, 3 },  -0.444089181829E-02 },
                                                { { 3, 2, 4, 3 },  -0.612942989364E-02 },
                                                { { 3, 3, 4, 3 },   0.350197853722E-07 },
                                                { { 4, 3, 4, 3 },   0.251137992170E-01 },
                                                { { 4, 4, 4, 3 },   0.323896546368E-07 },
                                                { { 1, 1, 4, 4 },   0.450421787348     },
                                                { { 2, 1, 4, 4 },   0.671517333359E-02 },
                                                { { 2, 2, 4, 4 },   0.195606443342     },
                                                { { 3, 1, 4, 4 },   0.185362625807E-08 },
                                                { { 3, 2, 4, 4 },   0.419234994857E-07 },
                                                { { 3, 3, 4, 4 },   0.382638069339     },
                                                { { 4, 4, 4, 4 },   0.370380122890     },
                                                { { 1, 1, 0, 0 },  -0.876082926130     },
                                                { { 2, 1, 0, 0 },   0.928246209082E-02 },
                                                { { 2, 2, 0, 0 },  -0.383002645306     },
                                                { { 3, 1, 0, 0 },   0.110478689971E-07 },
                                                { { 3, 2, 0, 0 },  -0.838075467983E-07 },
                                                { { 3, 3, 0, 0 },  -0.192723200850     },
                                                { { 4, 1, 0, 0 },   0.118275609870     },
                                                { { 4, 2, 0, 0 },   0.539573313879E-01 },
                                                { { 4, 3, 0, 0 },  -0.670886115563E-07 },
                                                { { 4, 4, 0, 0 },  -0.240135259399     },
                                                { { 0, 0, 0, 0 },   -6.71049529388     }
    };

    p.set("integrals_binary", chem::serialize<double>(integrals));
    p.set("site_types", "1,1,1,1");
    p.set("L", 4);
    p.set("irrep", 0);

    p.set("nsweeps",2);
    p.set("max_bond_dimension",100);

    // for SU2U1
    p.set("nelec", 2);
    p.set("spin", 0);

    // for 2U1

    p.set("u1_total_charge1", 1);
    p.set("u1_total_charge2", 1);

    std::vector<std::string> symmetries;
    #ifdef HAVE_SU2U1PG
    symmetries.push_back("su2u1pg");
    #endif
    #ifdef HAVE_SU2U1
    symmetries.push_back("su2u1");
    #endif
    #ifdef HAVE_TwoU1PG
    symmetries.push_back("2u1pg");
    #endif
    #ifdef HAVE_TwoU1
    symmetries.push_back("2u1");
    #endif

    for (auto&& s: symmetries)
    {
        maquis::cout << "Running test for symmetry " << s << std::endl;
        p.set("symmetry", s);

        maquis::DMRGInterface<double> interface(p);
        interface.optimize();

        BOOST_CHECK_CLOSE(interface.energy(), -7.90435750473166 , 1e-7);
    }


}

