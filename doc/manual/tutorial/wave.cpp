/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2008-04-14

  Copyright (C) 2008 Université Joseph Fourier (Grenoble I)

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
   \file wave.cpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2008-04-14
 */
#include <feel/options.hpp>
#include <feel/feelcore/application.hpp>

#include <feel/feelalg/backend.hpp>

#include <feel/feeldiscr/functionspace.hpp>
#include <feel/feeldiscr/region.hpp>
#include <feel/feeldiscr/operatorlinear.hpp>
#include <feel/feelpoly/im.hpp>

#include <feel/feelfilters/gmsh.hpp>
#include <feel/feelfilters/exporter.hpp>
#include <feel/feelfilters/gmshhypercubedomain.hpp>
#include <feel/feelpoly/polynomialset.hpp>


#include <feel/feelvf/vf.hpp>




inline
Feel::po::options_description
makeOptions()
{
    Feel::po::options_description waveoptions( "Wave options" );
    waveoptions.add_options()
    ( "dt", Feel::po::value<double>()->default_value( 1 ), "time step value" )
    ( "ft", Feel::po::value<double>()->default_value( 1 ), "final time value" )

    ( "order", Feel::po::value<int>()->default_value( 2 ), "order of time discretisation" )
    ( "diff", Feel::po::value<double>()->default_value( 1 ), "diffusion parameter" )
    ( "penal", Feel::po::value<double>()->default_value( 10 ), "penalisation parameter" )
    ( "penalbc", Feel::po::value<double>()->default_value( 10 ), "penalisation parameter for the weak boundary conditions" )
    ( "hsize", Feel::po::value<double>()->default_value( 0.5 ), "first h value to start convergence" )
    ( "bctype", Feel::po::value<int>()->default_value( 1 ), "0 = strong Dirichlet, 1 = weak Dirichlet" )
    ( "export", "export results(ensight, data file(1D)" )
    ( "export-mesh-only", "export mesh only in ensight format" )
    ( "export-matlab", "export matrix and vectors in matlab" )
    ;
    return waveoptions.add( Feel::feel_options() );
}
inline
Feel::AboutData
makeAbout()
{
    Feel::AboutData about( "wave" ,
                           "wave" ,
                           "0.2",
                           "nD(n=1,2,3) Wave on simplices or simplex products",
                           Feel::AboutData::License_GPL,
                           "Copyright (c) 2006, 2007 Université Joseph Fourier" );

    about.addAuthor( "Christophe Prud'homme", "developer", "christophe.prudhomme@feelpp.org", "" );
    return about;

}


namespace Feel
{
using namespace vf;
template<int Dim>struct ExactSolution {};
template<>
struct ExactSolution<1>
{
    typedef __typeof__( sin( M_PI*Px() ) ) type;
    typedef __typeof__( M_PI*M_PI*sin( M_PI*Px() ) ) wave_type;
};

template<>
struct ExactSolution<2>
{
    typedef __typeof__( sin( M_PI*Px() )*cos( M_PI*Py() ) ) type;
    typedef __typeof__( 2*M_PI*M_PI*sin( M_PI*Px() )*cos( M_PI*Py() ) ) wave_type;
};

template<>
struct ExactSolution<3>
{
    typedef __typeof__( sin( M_PI*Px() )*cos( M_PI*Py() )*cos( M_PI*Pz() ) ) type;
    typedef __typeof__( 3*M_PI*M_PI*sin( M_PI*Px() )*cos( M_PI*Py() )*cos( M_PI*Pz() ) ) wave_type;
};
/**
 * Wave Solver using discontinous approximation spaces
 *
 * solve \f$ -\Delta u = f\f$ on \f$\Omega\f$ and \f$u= g\f$ on \f$\Gamma\f$
 */
template<int Dim,
         int Order,
         typename Cont,
         template<uint16_type,uint16_type,uint16_type> class Entity,
         template<uint16_type> class FType>
class Wave
    :
public Application
{
    typedef Application super;
public:

    // -- TYPEDEFS --
    static const uint16_type imOrder = 2*Order;

    typedef double value_type;

    typedef Backend<value_type> backend_type;
    typedef boost::shared_ptr<backend_type> backend_ptrtype;

    /*matrix*/
    typedef typename backend_type::sparse_matrix_type sparse_matrix_type;
    typedef typename backend_type::sparse_matrix_ptrtype sparse_matrix_ptrtype;
    typedef typename backend_type::vector_type vector_type;
    typedef typename backend_type::vector_ptrtype vector_ptrtype;

    /*mesh*/
    typedef Entity<Dim,1,Dim> entity_type;
    typedef Mesh<entity_type> mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;

    typedef FunctionSpace<mesh_type, bases<Lagrange<0, Scalar> >,Discontinuous> p0_space_type;
    typedef typename p0_space_type::element_type p0_element_type;

    template<typename Conti = Cont>
    struct space
    {
        /*basis*/
        typedef bases<Lagrange<Order, FType> > basis_type;
        typedef bases<Lagrange<Order-1, Vectorial> > grad_basis_type;

        /*space*/
        typedef FunctionSpace<mesh_type, basis_type, Conti> type;
        typedef boost::shared_ptr<type> ptrtype;

        typedef FunctionSpace<mesh_type, grad_basis_type, Discontinuous> grad_type;
        typedef boost::shared_ptr<grad_type> grad_ptrtype;

        typedef typename type::element_type element_type;
        typedef typename grad_type::element_type grad_element_type;
    };
    typedef typename space<Cont>::type functionspace_type;
    typedef typename space<Cont>::ptrtype functionspace_ptrtype;
    typedef typename space<Cont>::element_type element_type;

    typedef typename space<Cont>::grad_type grad_functionspace_type;
    typedef typename space<Cont>::grad_ptrtype grad_functionspace_ptrtype;
    typedef typename space<Cont>::grad_element_type grad_element_type;

    /*quadrature*/
    //typedef IM_PK<Dim, imOrder, value_type> im_type;
    typedef IM<Dim, imOrder, value_type, Entity> im_type;

    /* export */
    typedef Exporter<mesh_type> export_type;
    typedef boost::shared_ptr<export_type> export_ptrtype;

    Wave()
        :
        super(),
        M_backend( backend_type::build( this->vm() ) ),
        meshSize( this->vm()["hsize"].template as<double>() ),
        exporter( Exporter<mesh_type>::New( this->vm(), this->about().appName() ) ),
        timers(),
        stats()
    {
    }

    /**
     * create the mesh using mesh size \c meshSize
     */
    mesh_ptrtype createMesh( double meshSize, double ymin = 0, double ymax = 1 );

    /**
     * run the convergence test
     */
    void run();

private:



    /**
     * solve the system
     */
    void solve( sparse_matrix_ptrtype& D, element_type& u, vector_ptrtype& F );


    /**
     * export results to ensight format (enabled by  --export cmd line options)
     */
    template<typename f1_type, typename f2_type, typename f3_type>
    void exportResults( double time,
                        f1_type& u,
                        f2_type& v,
                        f3_type& e );

private:

    backend_ptrtype M_backend;

    double meshSize;

    export_ptrtype exporter;

    std::map<std::string,std::pair<boost::timer,double> > timers;
    std::map<std::string,double> stats;
}; // Wave

template<int Dim, int Order, typename Cont, template<uint16_type,uint16_type,uint16_type> class Entity, template<uint16_type> class FType>
typename Wave<Dim,Order,Cont,Entity,FType>::mesh_ptrtype
Wave<Dim,Order,Cont,Entity,FType>::createMesh( double meshSize, double ymin, double ymax )
{
    timers["mesh"].first.restart();
    mesh_ptrtype mesh( new mesh_type );
    //mesh->setRenumber( false );

    GmshHypercubeDomain td( entity_type::nDim,entity_type::nOrder,entity_type::nRealDim,entity_type::is_hypercube );
    td.setCharacteristicLength( meshSize );

    if ( Dim >=2 )
        td.setY( std::make_pair( ymin, ymax ) );

    std::string fname = td.generate( entity_type::name().c_str() );

    ImporterGmsh<mesh_type> import( fname );
    mesh->accept( import );
    timers["mesh"].second = timers["mesh"].first.elapsed();
    LOG(INFO) << "[timer] createMesh(): " << timers["mesh"].second << "\n";
    return mesh;
} // Wave::createMesh


template<int Dim, int Order, typename Cont, template<uint16_type,uint16_type,uint16_type> class Entity, template<uint16_type> class FType>
void
Wave<Dim, Order, Cont, Entity, FType>::run()
{
    if ( this->vm().count( "help" ) )
    {
        std::cout << this->optionsDescription() << "\n";
        return;
    }

    //    int maxIter = 10.0/meshSize;
    using namespace Feel::vf;


    this->changeRepository( boost::format( "%1%/%2%/P%3%/h_%4%/" )
                            % this->about().appName()
                            % entity_type::name()
                            % Order
                            % this->vm()["hsize"].template as<double>()
                          );

    /*
     * First we create the mesh
     */
    mesh_ptrtype mesh = createMesh( meshSize );
    stats["nelt"] = mesh->elements().size();

    /*
     * The function space and some associate elements are then defined
     */
    timers["init"].first.restart();
    functionspace_ptrtype Xh = space<Cont>::type::New( mesh );
    grad_functionspace_ptrtype gXh = grad_functionspace_type::New( mesh );
    //Xh->dof()->showMe();
    element_type u( Xh, "u" );
    grad_element_type gu( gXh, "gu" );
    element_type v( Xh, "v" );

    timers["init"].second = timers["init"].first.elapsed();
    stats["ndof"] = Xh->nDof();

    if ( this->vm().count( "export-mesh-only" ) )
        this->exportResults( 0., u, u, u );

    im_type im;

    value_type penalisation = this->vm()["penal"].template as<value_type>();
    value_type penalisation_bc = this->vm()["penalbc"].template as<value_type>();
    int bctype = this->vm()["bctype"].template as<int>();
    value_type dt = this->vm()["dt"].template as<value_type>();
    value_type ft = this->vm()["ft"].template as<value_type>();
    value_type order = this->vm()["order"].template as<int>();

    double factor = 1./( dt*dt );
    OperatorLinear<functionspace_type,functionspace_type> Mass( Xh, Xh, M_backend );
    Mass = integrate( elements( mesh ),  idt( u )*id( v )/dt );
    Mass.close();

    OperatorLinear<functionspace_type,functionspace_type> Stiff( Xh, Xh, M_backend );

    Stiff =
        integrate( elements( mesh ),  factor*idt( u )*id( v )+gradt( u )*trans( grad( v ) ) ) +
#if 0
        integrate( internalfaces( mesh ),
                   // - {grad(u)} . [v]
                   -averaget( gradt( u ) )*jump( id( v ) )
                   // - [u] . {grad(v)}
                   -average( grad( v ) )*jumpt( idt( u ) )
                   // penal*[u] . [v]/h_face
                   + penalisation* ( trans( jumpt( idt( u ) ) )*jump( id( v ) ) )/hFace() ) +
#endif
        integrate( boundaryfaces( mesh ),
                   //integrate( markedfaces(mesh,1),
                   ( - trans( id( v ) )*( gradt( u )*N() )
                     - trans( idt( u ) )*( grad( v )*N() )
                     + penalisation_bc*trans( idt( u ) )*id( v )/hFace() ) );

    Stiff.close();


    FsFunctionalLinear<functionspace_type> F( Xh, M_backend );
    F = integrate( elements( mesh ),  id( v ) );

    Stiff.applyInverse( u, F );

    //Stiff.add( 1.0, Mass );

    //OperatorCompose<functionspace_type,functionspace_type> MiK( Mass

    exportResults( 0.0, u, u, u );

    vector_ptrtype L( M_backend->newVector( Xh ) );
    element_type un2( Xh, "un2" );
    element_type un1( Xh, "un1" );
    element_type un( Xh, "un" );
    element_type vn( Xh, "un" );
    un2.zero();
    un1.zero();
    un.zero();

    for ( double time = dt; time <= ft; time += dt )
    {
        form1( _test=Xh, _vector=L, _init=true ) = integrate( elements( mesh ),  id( v )+factor*( 2.*idv( un )-idv( un1 ) )*id( v ) );

        L->close();

        solve( Stiff.matPtr(), u, L );

        exportResults( time, u, un, un1 );

        un2 = un1;
        un1 = un;
        un = u;
    }

    //gu = vf::project( gXh, elements(mesh), gradv( u ) );
    //exportResults( ft+dt, gu, u, u );
} // Wave::run

template<int Dim, int Order, typename Cont, template<uint16_type,uint16_type,uint16_type> class Entity, template<uint16_type> class FType>
void
Wave<Dim, Order, Cont, Entity, FType>::solve( sparse_matrix_ptrtype& D,
        element_type& u,
        vector_ptrtype& F )
{
    timers["solver"].first.restart();


    //backend->set_symmetric( is_sym );

    vector_ptrtype U( M_backend->newVector( u.functionSpace() ) );
    M_backend->solve( D, D, U, F );
    u = *U;

    timers["solver"].second = timers["solver"].first.elapsed();
    LOG(INFO) << "[timer] solve: " << timers["solver"].second << "\n";
} // Wave::solve


template<int Dim, int Order, typename Cont, template<uint16_type,uint16_type,uint16_type> class Entity, template<uint16_type> class FType>
template<typename f1_type, typename f2_type, typename f3_type>
void
Wave<Dim, Order, Cont, Entity,FType>::exportResults( double time,
        f1_type& U,
        f2_type& V,
        f3_type& E )
{
    timers["export"].first.restart();

    LOG(INFO) << "exportResults starts\n";

    exporter->step( time )->setMesh( U.functionSpace()->mesh() );

    //exporter->step(time)->setMesh( this->createMesh( meshSize/2, 0.5, 1 ) );
    //exporter->step(time)->setMesh( this->createMesh( meshSize/Order, 0, 1 ) );
    //exporter->step(time)->setMesh( this->createMesh( meshSize, 0, 1 ) );
    if ( !this->vm().count( "export-mesh-only" ) )
    {
        exporter->step( time )->addRegions();
        exporter->step( time )->add( "u", U );
        exporter->step( time )->add( "v", V );
        exporter->step( time )->add( "e", E );
    }

    exporter->save();


    if ( Dim == 1 )
    {
        std::ostringstream fname_u;
        fname_u << "u-" << Application::processId() << "." << boost::format( "%.2f" ) % time << ".dat";
        std::ofstream ofs3( fname_u.str().c_str() );
        typename mesh_type::element_iterator it = U.functionSpace()->mesh()->beginElementWithProcessId( Application::processId() );
        typename mesh_type::element_iterator en = U.functionSpace()->mesh()->endElementWithProcessId( Application::processId() );

        if ( !U.areGlobalValuesUpdated() )
            U.updateGlobalValues();

        for ( ; it!=en; ++it )
        {
            for ( size_type i = 0; i < space<Cont>::type::basis_type::nLocalDof; ++i )
            {
                size_type dof0 = boost::get<0>( U.functionSpace()->dof()->localToGlobal( it->id(), i ) );
                ofs3 << std::setw( 5 ) << it->id() << " "
                     << std::setw( 5 ) << i << " "
                     << std::setw( 5 ) << dof0 << " "
                     << std::setw( 15 ) << U.globalValue( dof0 ) << " ";
                value_type a = it->point( 0 ).node()[0];
                value_type b = it->point( 1 ).node()[0];

                if ( i == 0 )
                    ofs3 << a;

                else if ( i == 1 )
                    ofs3 <<  b;

                else
                    ofs3 <<  a + ( i-1 )*( b-a )/( space<Continuous>::type::basis_type::nLocalDof-1 );

                ofs3 << "\n";

            }
        }

        ofs3.close();

        std::ostringstream fname_v;
        fname_v << "values-" << Application::processId() << "." << boost::format( "%.2f" ) % time << ".dat";
        std::ofstream ofs2( fname_v.str().c_str() );
        it = V.functionSpace()->mesh()->beginElementWithProcessId( Application::processId() );
        en = V.functionSpace()->mesh()->endElementWithProcessId(  Application::processId() );

        if ( !V.areGlobalValuesUpdated() ) V.updateGlobalValues();

        if ( !E.areGlobalValuesUpdated() ) E.updateGlobalValues();

        for ( ; it!=en; ++it )
        {
            for ( size_type i = 0; i < space<Continuous>::type::basis_type::nLocalDof; ++i )
            {
                size_type dof0 = boost::get<0>( V.functionSpace()->dof()->localToGlobal( it->id(), i ) );
                ofs2 << std::setw( 5 ) << it->id() << " "
                     << std::setw( 5 ) << i << " "
                     << std::setw( 5 ) << dof0 << " "
                     << std::setw( 15 ) << V.globalValue( dof0 ) << " "
                     << std::setw( 15 ) << E.globalValue( dof0 ) << " ";
                value_type a = it->point( 0 ).node()[0];
                value_type b = it->point( 1 ).node()[0];

                if ( i == 0 )
                    ofs2 << a;

                else if ( i == 1 )
                    ofs2 <<  b;

                else
                    ofs2 <<  a + ( i-1 )*( b-a )/( space<Continuous>::type::basis_type::nLocalDof-1 );

                ofs2 << "\n";

            }
        }

    }

    timers["export"].second = timers["export"].first.elapsed();
    LOG(INFO) << "[timer] exportResults(): " << timers["export"].second << "\n";
} // Wave::export
} // Feel




int
main( int argc, char** argv )
{
    using namespace Feel;
    Environment env( _argc=argc, _argv=argv,_desc=makeOptions(),_about=makeAbout() );

    /* change parameters below */
    const int nDim = 1;
    const int nOrder = 2;
    typedef Continuous MyContinuity;
    //typedef Discontinuous MyContinuity;
    //typedef Feel::Wave<nDim, nOrder, MyContinuity, Hypercube, Scalar> wave_type;

    typedef Feel::Wave<nDim, nOrder, MyContinuity, Simplex, Scalar> wave_type;

    /* define and run application */
    wave_type wave;

    wave.run();
}





