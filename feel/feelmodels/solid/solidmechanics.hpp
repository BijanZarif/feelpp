/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=cpp:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Vincent Chabannes <vincent.chabannes@feelpp.org>
       Date: 2014-06-04

  Copyright (C) 2014 Université Joseph Fourier (Grenoble I)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3.0 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file solidmechanics.hpp
   \author Vincent Chabannes <vincent.chabannes@feelpp.org>
   \date 2014-06-04
 */

#ifndef FEELPP_SOLIDMECHANICS_HPP
#define FEELPP_SOLIDMECHANICS_HPP 1

#include <feel/feelmodels/solid/solidmecbase.hpp>


namespace Feel
{
namespace FeelModels
{

template< typename ConvexType, typename BasisDisplacementType,bool UseCstMechProp=false >
class SolidMechanics : public SolidMechanicsBase<ConvexType,BasisDisplacementType,UseCstMechProp>,
                       public boost::enable_shared_from_this< SolidMechanics<ConvexType,BasisDisplacementType,UseCstMechProp> >
{
public:
    typedef SolidMechanicsBase<ConvexType,BasisDisplacementType,UseCstMechProp> super_type;

    typedef SolidMechanics<ConvexType,BasisDisplacementType,UseCstMechProp> self_type;
    typedef boost::shared_ptr<self_type> self_ptrtype;

    using element_displacement_type = typename super_type::element_displacement_type;

    SolidMechanics( std::string prefix,
                    bool __buildMesh=true,
                    WorldComm const& _worldComm=Environment::worldComm(),
                    std::string subPrefix="",
                    std::string appliShortRepository=soption(_name="exporter.directory") );

    //___________________________________________________________________________________//
    // load config files
    void loadConfigBCFile();
    void loadConfigPostProcess();
    void loadConfigMeshFile( std::string const& geofilename );
    void loadConfigMeshFile1dReduced( std::string const& geofilename );

    // update for use
    void init( bool buildModelAlgebraicFactory = true );
    void solve( bool upVelAcc=true );

    //___________________________________________________________________________________//
    // assembly using bc
    void updateNewtonInitialGuess( vector_ptrtype& U ) const;

    void updateBCDirichletStrongResidual( vector_ptrtype& R ) const;
    void updateBCNeumannResidual( vector_ptrtype& R ) const;
    void updateBCFollowerPressureResidual(element_displacement_type const& u, vector_ptrtype& R ) const;
    void updateBCRobinResidual( element_displacement_type const& u, vector_ptrtype& R ) const;
    void updateSourceTermResidual( vector_ptrtype& R ) const;

    void updateBCDirichletStrongJacobian(sparse_matrix_ptrtype& J) const;
    void updateBCFollowerPressureJacobian(element_displacement_type const& u, sparse_matrix_ptrtype& J) const;
    void updateBCRobinJacobian( sparse_matrix_ptrtype& J) const;

    void updateSourceTermLinearPDE( vector_ptrtype& F ) const;
    void updateBCNeumannLinearPDE( vector_ptrtype& F ) const;
    void updateBCRobinLinearPDE( sparse_matrix_ptrtype& A, vector_ptrtype& F ) const;
    void updateBCDirichletStrongLinearPDE(sparse_matrix_ptrtype& A, vector_ptrtype& F) const;

private :
    map_vector_field<super_type::nDim,1,2> M_bcDirichlet;
    map_scalar_field<2> M_bcDirichletX,M_bcDirichletY,M_bcDirichletZ;
    map_scalar_field<2> M_bcNeumannScalar,M_bcInterfaceFSI;
    map_vector_field<super_type::nDim,1,2> M_bcNeumannVectorial;
    map_vector_fields<super_type::nDim,1,2> M_bcRobin;
    map_vector_field<super_type::nDim,1,2> M_volumicForcesProperties;
};

} // namespace FeelModels
} // namespace Feel

#endif // INCLUDE_SOLIDMECHANICS_HPP
