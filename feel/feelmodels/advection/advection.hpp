/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=cpp:et:sw=4:ts=4:sts=4

 This file is part of the Feel library

 Author(s): Thibaut Metivet <thibaut.metivet@univ-grenoble-alpes.fr>
 Date: 2016-09-20

 Copyright (C) 2016 Université Joseph Fourier (Grenoble I)

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
 \file advection.hpp
 \author Thibaut Metivet <thibaut.metivet@univ-grenoble-alpes.fr>
 \date 2016-09-20
 */

#ifndef _ADVECTION_HPP
#define _ADVECTION_HPP 1

#include <feel/feelmodels/advection/advectionbase.hpp>

namespace Feel {
namespace FeelModels {

template< 
    typename ConvexType, typename BasisAdvectionType, 
    typename BasisDiffusionReactionType = Lagrange<0, Scalar, Discontinuous>
        >
class Advection
    : public AdvectionBase<ConvexType, BasisAdvectionType, BasisDiffusionReactionType>
    , public boost::enable_shared_from_this< Advection<ConvexType, BasisAdvectionType, BasisDiffusionReactionType> >
{
public:
    typedef AdvectionBase<ConvexType, BasisAdvectionType, BasisDiffusionReactionType> super_type;

    typedef Advection<ConvexType, BasisAdvectionType, BasisDiffusionReactionType> self_type;
    typedef boost::shared_ptr<self_type> self_ptrtype;

    typedef typename super_type::space_advection_ptrtype space_advection_ptrtype;
    typedef typename super_type::element_advection_ptrtype element_advection_ptrtype;

    //--------------------------------------------------------------------//
    // Constructor
    Advection( 
            std::string const& prefix,
            WorldComm const& _worldComm = Environment::worldComm(),
            std::string const& subPrefix = "",
            std::string const& rootRepository = ModelBase::rootRepositoryByDefault() );

    static self_ptrtype New( 
            std::string const& prefix,
            WorldComm const& _worldComm = Environment::worldComm(),
            std::string const& subPrefix = "",
            std::string const& rootRepository = ModelBase::rootRepositoryByDefault() );
    //--------------------------------------------------------------------//
    // Initialization
    void init( bool buildModelAlgebraicFactory = true );
    void loadConfigBCFile();
    //--------------------------------------------------------------------//
    // BC and source term assembly
    void updateWeakBCLinearPDE(sparse_matrix_ptrtype& A, vector_ptrtype& F,bool buildCstPart) const;
    void updateBCStrongDirichletLinearPDE(sparse_matrix_ptrtype& A, vector_ptrtype& F) const;
    void updateSourceTermLinearPDE(element_advection_ptrtype& fieldSource, bool buildCstPart) const;

    bool hasSourceTerm() const;

protected:
    // Boundary conditions
    map_scalar_field<2> M_bcDirichlet;
    map_scalar_field<2> M_bcNeumann;
    map_scalar_fields<2> M_bcRobin;

    map_scalar_field<2> M_sources;

};
    

} // namespace FeelModels
} // namespace Feel

#endif
