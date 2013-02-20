/* -*- mode: c++ -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2009-08-10

  Copyright (C) 2009 Universit� Joseph Fourier (Grenoble I)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file crbscm.cpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2009-08-10
 */

#include <feel/feelcore/feel.hpp>
#include <feel/feelalg/solvereigen.hpp>
#include <feel/feelcrb/crbscm.hpp>

namespace Feel
{
Feel::po::options_description
crbSCMOptions( std::string const& prefix )
{
    Feel::po::options_description crbscmoptions( "CRB SCM Options" );
    crbscmoptions.add_options()
    ( "crb.scm.sampling-size"   , Feel::po::value<int>()->default_value( 100 ),       "Offline SCM sampling size " )
    ( "crb.scm.tol"   , Feel::po::value<double>()->default_value( 1e-6 ),       "Offline SCM tolerance" )
    ( "crb.scm.iter-max"   , Feel::po::value<int>()->default_value( 10 ),       "Offline SCM max number of K iterations" )
    ( "crb.scm.Mplus" , Feel::po::value<int>()->default_value( 10 ),       "M+ value" )
    ( "crb.scm.Malpha" , Feel::po::value<int>()->default_value( 10 ),       "M_alpha value" )
    ( "crb.scm.level" , Feel::po::value<int>()->default_value( 1 ),       "level for recursion in lower bound computations" )
    ( "crb.scm.strategy" , Feel::po::value<int>()->default_value( 2 ),       "scm strategy (0=patera, 1=maday, 2=prudhomme" )
    ( "crb.scm.rebuild-database" , Feel::po::value<bool>()->default_value( 0 ), "rebuild database (if it already exists)" )
    ( "crb.scm.do-scm-for-mass-matrix" , Feel::po::value<bool>()->default_value( 0 ), "do scm for bilinear form m(.,.;mu) and not for a(.,.;mu) " )
    ( "crb.scm.print-matrix" , Feel::po::value<bool>()->default_value( 0 ), "print matrix " )
    ( "crb.scm.solvereigen-tol" ,  Feel::po::value<double>()->default_value( 1e-10 ), "solver eigen tolerance " )
    ( "crb.scm.solvereigen-maxiter" ,  Feel::po::value<int>()->default_value( 10000 ), "solver eigen maxiter " )
    ( "crb.scm.solvereigen-nev" ,  Feel::po::value<int>()->default_value( 1 ), "solver eigen nev " )
    ( "crb.scm.solvereigen-ncv" ,  Feel::po::value<int>()->default_value( 3 ), "solver eigen ncv " )
    ( "crb.scm.solvereigen-solver-type" ,  Feel::po::value<int>()->default_value( 5 ), "solver eigen type " )
    ;



    crbscmoptions
    .add( solvereigen_options( "crb.scm" ) );

    return crbscmoptions;
}
}

//here is the list of choice for solvereigen-solver-type :
//0 : POWER
//1 : LAPACK
//2 : SUBSPACE
//3 : ARNOLDI
//4 : LANCZOS
//5 : KRYLOVSCHUR

