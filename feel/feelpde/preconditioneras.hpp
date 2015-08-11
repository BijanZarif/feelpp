/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*-
   -*- vim: set ft=cpp fenc=utf-8 sw=4 ts=4 sts=4 tw=80 et cin cino=N-s,c0,(0,W4,g0:

   This file is part of the Feel++ library

   Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   Goncalo Pena  <gpena@mat.uc.pt>
Date: 02 Oct 2014

Copyright (C) 2014-2015 Feel++ Consortium

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
#ifndef FEELPP_PRECONDITIONERAS_HPP
#define FEELPP_PRECONDITIONERAS_HPP 1


#include <feel/feeldiscr/operatorinterpolation.hpp>
#include <feel/feelalg/backend.hpp>
#include <feel/feelalg/operator.hpp>
#include <feel/feelalg/preconditioner.hpp>
//#include <feel/feelpde/operatoras.hpp>
#include <feel/feelpde/boundaryconditions.hpp>
#include <feel/feelalg/backendpetsc.hpp>

namespace Feel
{
template< typename space_type, typename coef_space_type >
    class PreconditionerAS : public Preconditioner<typename space_type::value_type>
{
    typedef Preconditioner<typename space_type::value_type> super;
public:

    static const uint16_type Dim = space_type::nDim;
    typedef typename space_type::value_type value_type;

    enum Type
    {
        AS    = 0, // augmentation free preconditioner
        SIMPLE = 2 // 
    };
    typedef typename backend_type::sparse_matrix_type sparse_matrix_type;
    typedef typename backend_type::sparse_matrix_ptrtype sparse_matrix_ptrtype;

    typedef typename backend_type::vector_type vector_type;
    typedef typename backend_type::vector_ptrtype vector_ptrtype;

    typedef boost::shared_ptr<space_type> space_ptrtype;
    typedef boost::shared_ptr<coef_space_type> coef_space_ptrtype;
    typedef typename space_type::indexsplit_ptrtype  indexsplit_ptrtype;
    typedef typename space_type::mesh_type mesh_type;
    typedef typename space_type::mesh_ptrtype mesh_ptrtype;
    typedef typename space_type::element_type element_type;
    // Vh
    typedef typename space_type::template sub_functionspace<0>::type potential_space_type;
    typedef typename space_type::template sub_functionspace<1>::type lagrange_space_type;
    // Qh
    typedef typename space_type::template sub_functionspace<0>::ptrtype potential_space_ptrtype;
    typedef typename space_type::template sub_functionspace<1>::ptrtype lagrange_space_ptrtype;
    // Qh3 - temporary version
    typedef Lagrange<1,Vectorial> lag_v_type;
    typedef FunctionSpace<mesh_type, bases<lag_v_type>> lag_v_space_type;
    typedef boost::shared_ptr<lag_v_space_type> lag_v_space_ptrtype;

    // Elements
    typedef typename potential_space_type::element_type potential_element_type;
    typedef typename lagrange_space_type::element_type lagrange_element_type;
    typedef typename coef_space_type::element_type element_coef_type;
    typedef typename lag_v_space_type::element_type lag_v_element_type; 

    // operatrors
    typedef OperatorMatrix<value_type> op_mat_type;
    typedef boost::shared_ptr<op_mat_type> op_mat_ptrtype;

    typedef OperatorInverse<op_mat_type> op_inv_type;
    typedef boost::shared_ptr<op_inv_type> op_inv_ptrtype;

    typedef OperatorBase<value_type> op_type;
    typedef boost::shared_ptr<op_type> op_ptrtype;

    /**
     * \param t Kind of prec
     * \param Xh potential space type
     * \param Mh Permeability space type
     * \param bcFlags the boundary conditions flags
     * \param s name of backend
     * \param k value
     */
    PreconditionerAS( std::string t,
                      space_ptrtype Xh,
                      coef_space_ptrtype Mh, 
                      BoundaryConditions bcFlags,
                      std::string const& s,
                      double k
                      );

    Type type() const { return M_type; }
    void setType( std::string t );

    void update( sparse_matrix_ptrtype A, sparse_matrix_ptrtype L, element_coef_type mu );

    void apply( const vector_type & X, vector_type & Y ) const
    {
        this->applyInverse(X,Y); 
    }

    int applyInverse ( const vector_type& X, vector_type& Y ) const;

    virtual ~PreconditionerAS(){};

private:

    Type M_type;

    space_ptrtype M_Xh;
    potential_space_ptrtype M_Vh;
    lagrange_space_ptrtype M_Qh;
    lag_v_space_ptrtype M_Qh3;
    coef_space_ptrtype M_Mh;

    sparse_matrix_ptrtype 
        M_P, M_Pt,
        M_C, M_Ct;

    op_ptrtype SimpleOp;
    op_mat_ptrtype opMat1,
                   opMat2;
    op_inv_ptrtype opMatInv;

    mutable vector_ptrtype
        A,B,C,
        M_r, M_r_t,
        M_uout,
        M_diagPm,
        M_t, M_s,
        M_y, M_y_t,
        M_z, M_z_t;

    mutable potential_element_type U;
    element_coef_type M_mu;

    op_ptrtype
               M_lgqOp,    // \bar{L} + gamma \bar{Q}
               M_lOp      // L
               ;

    BoundaryConditions M_bcFlags;
    std::string M_prefix;

    value_type M_k; // k
    value_type M_g; // gamma
};

template < typename space_type, typename coef_space_type >
PreconditionerAS<space_type,coef_space_type>::PreconditionerAS( std::string t,
                                                              space_ptrtype Xh, 
                                                              coef_space_ptrtype Mh, 
                                                              BoundaryConditions bcFlags,
                                                              std::string const& p,
                                                              double k)
    :
        M_type( AS ),
        M_Xh( Xh ),
        M_Vh(Xh->template functionSpace<0>() ),
        M_Qh(Xh->template functionSpace<1>() ),
        M_Mh( Mh ),
        A(backend()->newVector(M_Vh)),
        B(backend()->newVector(M_Vh)),
        C(backend()->newVector(M_Vh)),
        M_r(backend()->newVector(M_Vh)),
        M_r_t(backend()->newVector(M_Vh)),
        M_uout(backend()->newVector(M_Vh)),
        M_diagPm(backend()->newVector(M_Vh)),
        M_t(backend()->newVector(M_Vh)),
        U( M_Vh, "U" ),
        M_mu(M_Mh, "mu"),
        M_bcFlags( bcFlags ),
        M_prefix( p ),
        M_k(k),
        M_g(1.-k*k)
{
    tic();
    LOG(INFO) << "[PreconditionerAS] setup starts";
    M_Qh3 = lag_v_space_type::New(Xh->mesh());
    M_s = backend()->newVector(M_Qh3);
    M_y = backend()->newVector(M_Qh3);

    // Create the interpolation and keep only the matrix
    /// ** That is _really_ long !
    auto pi_curl = opInterpolation(_domainSpace=M_Qh3, _imageSpace=M_Vh);
    M_P = pi_curl->matPtr();
    M_Pt= backend()->newMatrix(M_Qh3,M_Vh);
    M_P->transpose(M_Pt,MATRIX_TRANSPOSE_UNASSEMBLED);

    // todo: create M_C and M_Ct
    //std::iota( M_Vh_indices.begin(), M_Vh_indices.end(), 0 );
    //std::iota( M_Qh_indices.begin(), M_Qh_indices.end(), M_Vh->nLocalDofWithGhost() );

    this->setType ( t );
    toc( "[PreconditionerAS] setup done ", FLAGS_v > 0 );
}

template < typename space_type, typename coef_space_type >
    void
PreconditionerAS<space_type,coef_space_type>::setType( std::string t )
{
    if ( t == "AS")     M_type = AS;
    if ( t == "SIMPLE") M_type = SIMPLE;
}

template < typename space_type, typename coef_space_type >
//template< typename Expr_convection, typename Expr_bc >
    void
PreconditionerAS<space_type,coef_space_type>::update( sparse_matrix_ptrtype Pm, 
                                                    sparse_matrix_ptrtype L,
                                                    element_coef_type mu )
{
    // Mettre à jour les matrices
    tic();

    if(this->type() == AS){
    // A = Pm
    //M_diagPm = compose(diag ( op (Pm, "blockms.11")),op(Pm,"blockms.11.diag")) ;
    backend()->diag(Pm,M_diagPm);
    M_lOp    = op( L, "blockms.11.2");

    auto u = M_Qh3->element("u");
    auto f21 = form2(M_Qh3, M_Qh3); // L + g Q - see 14.a
    f21 = integrate(_range=elements(M_Qh3->mesh()),
                    _expr=1./idv(mu)*inner(grad(u),gradt(u)) // should be wrong
                    + M_g*inner(id(u),idt(u)));
    M_lgqOp  = op( f21.matrixPtr(), "blockms.11.1");
    //M_pm = f21.matrixPtr();
#if 0
    this->createMatrices();

    if ( type() == AS )
    {
        tic();
        afpOp->update( expr_b, g );
        toc( "Preconditioner::update AS", FLAGS_v > 0 );

    }
#endif
    }
    else if(this->type() == SIMPLE)
    {
        auto uu = M_Vh->element("uu");
        auto f22 = form2(M_Vh, M_Vh); 
        f22 = integrate(_range=elements(M_Vh->mesh()),
                        _expr=inner(id(uu),idt(uu)));
        SimpleOp = op( f22.matrixPtr(),"blockms.11.1");
    }
    toc( "PreconditionerAS::update", FLAGS_v > 0 );
}



template < typename space_type, typename coef_space_type >
int
PreconditionerAS<space_type,coef_space_type>::applyInverse ( const vector_type& X /*R*/, vector_type& Y /*W*/) const
{
    /*
     * We solve Here P_v w = r
     * With P_v^-1 = diag(P_m)^-1
     *              + P (\bar L + g \bar Q) P^t
     *              + C (L^-1) C^T
     */
#if 1
    // Decompose les éléments
    tic();
    U = X;
    U.close();
    toc("Element created",FLAGS_v>0);

    // résout l'équation 12
    if ( this->type() == AS )
    {
        tic();
        // RHS calculation
        auto tmp = backend()->newVector(M_Vh);
        *tmp = X;
        tmp->close();
        // A = diag(Pm)^-1*r
        M_diagPm->pointwiseDivide(*tmp,*A);
        // s = P^t r
        M_Pt->multVector(tmp,M_s);
        // 14.a
        M_lgqOp->applyInverse(M_s,M_y);
        // B = P*y
        M_P->multVector(M_y,B);

        // Not yet available
#if 0
        // t = C^t r
        M_Ct->multVector(tmp,M_t);
        // 14.b
        M_lOp->applyInverse(M_t,M_z);
        // C = C z
        M_C->multVector(M_z,C);
        C->scale(1./M_g);
        A->add(*C);
#endif
        A->add(*B);
        toc("15 assembled",FLAGS_v>0);
        *M_uout = *A; // 15;
    }
    else if( this->type() == SIMPLE ){
        SimpleOp->applyInverse(X, Y);
        *M_uout = Y;
    }
    else{
        Y=X;
        *M_uout = Y;
    }

    tic();
    Y=*M_uout;
    Y.close();
    toc("PreconditionerAS::applyInverse" );
#endif
    return 0;
}

namespace meta
{
template< typename space_type , typename coef_space_type >
    struct blockas
{
    typedef PreconditionerAS<space_type, coef_space_type> type;
    typedef boost::shared_ptr<type> ptrtype;
};
}
BOOST_PARAMETER_MEMBER_FUNCTION( ( typename meta::blockas<
                                   typename parameter::value_type<Args, tag::space>::type::element_type,
                                   typename parameter::value_type<Args, tag::space2>::type::element_type
                                   >::ptrtype ),
                                 blockas,
                                 tag,
                                 ( required
                                   ( space, *)
                                   ( space2, *)
                                 )
                                 ( optional
                                   ( type, *, "AS")
                                   ( prefix, *( boost::is_convertible<mpl::_,std::string> ), "" )
                                   ( bc, *, (BoundaryConditions ()) )
                                   ( h, (double), 0.0 ) // for k ...
                                 )
                               )
{
#if 1
    typedef typename meta::blockas<
        typename parameter::value_type<Args, tag::space>::type::element_type,
        typename parameter::value_type<Args, tag::space2>::type::element_type
                     >::ptrtype pas_ptrtype;

    typedef typename meta::blockas<
        typename parameter::value_type<Args, tag::space>::type::element_type,
        typename parameter::value_type<Args, tag::space2>::type::element_type
                     >::type pas_type;
    pas_ptrtype p( new pas_type( type, space, space2, bc, prefix, h ) );
    return p;
#endif
} // 
} // Feel
#endif
