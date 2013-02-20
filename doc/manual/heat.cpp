

/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2008-02-07

  Copyright (C) 2008-2010 Universite Joseph Fourier (Grenoble I)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
   \file laplacian.cpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2010-07-15
 */
/** include predefined feel command line options */
#include <feel/options.hpp>

/** include linear algebra backend */
#include <feel/feelalg/backend.hpp>

/** include function space class */
#include <feel/feeldiscr/functionspace.hpp>

/** include helper function to define \f$P_0\f$ functions associated with regions  */
#include <feel/feeldiscr/region.hpp>

/** include integration methods */
#include <feel/feelpoly/im.hpp>

/** include gmsh mesh importer */
#include <feel/feelfilters/gmsh.hpp>

/** include exporter factory class */
#include <feel/feelfilters/exporter.hpp>

/** include  polynomialset header */
#include <feel/feelpoly/polynomialset.hpp>

/** include  the header for the variational formulation language (vf) aka FEEL++ */
#include <feel/feelvf/vf.hpp>

/** use Feel namespace */
using namespace Feel;
using namespace Feel::vf;

/**
 * This routine returns the list of options using the
 * boost::program_options library. The data returned is typically used
 * as an argument of a Feel::Application subclass.
 *
 * \return the list of options
 */
inline
po::options_description
makeOptions()
{
    po::options_description laplacianoptions( "Laplacian options" );
    laplacianoptions.add_options()
    ( "hsize", po::value<double>()->default_value( 0.5 ), "mesh size" )
    ( "shape", Feel::po::value<std::string>()->default_value( "hypercube" ), "shape of the domain (either simplex or hypercube)" )
    ( "nu", po::value<double>()->default_value( 1 ), "grad.grad coefficient" )
    ( "weakdir", po::value<int>()->default_value( 1 ), "use weak Dirichlet condition" )
    ( "penaldir", Feel::po::value<double>()->default_value( 10 ),
      "penalisation parameter for the weak boundary Dirichlet formulation" )
    ;
    return laplacianoptions.add( Feel::feel_options() );
}

/**
 * This routine defines some information about the application like
 * authors, version, or name of the application. The data returned is
 * typically used as an argument of a Feel::Application subclass.
 *
 * \return some data about the application.
 */
inline
AboutData
makeAbout()
{
    AboutData about( "laplacian" ,
                     "laplacian" ,
                     "0.2",
                     "nD(n=1,2,3) Laplacian on simplices or simplex products",
                     Feel::AboutData::License_GPL,
                     "Copyright (c) 2008-2009 Universite Joseph Fourier" );

    about.addAuthor( "Christophe Prud'homme", "developer", "christophe.prudhomme@feelpp.org", "" );
    return about;

}


/**
 * \class Laplacian
 *
 * Laplacian Solver using continuous approximation spaces
 * solve \f$ -\Delta u = f\f$ on \f$\Omega\f$ and \f$u= g\f$ on \f$\Gamma\f$
 *
 * \tparam Dim the geometric dimension of the problem (e.g. Dim=1, 2 or 3)
 */
template<int Dim>
class Laplacian
    :
public Simget
{
    typedef Simget super;
public:

    //! Polynomial order \f$P_2\f$
    static const uint16_type Order = 2;

    //! numerical type is double
    typedef double value_type;

    //! linear algebra backend factory
    typedef Backend<value_type> backend_type;
    //! linear algebra backend factory shared_ptr<> type
    typedef boost::shared_ptr<backend_type> backend_ptrtype;


    //! sparse matrix type associated with backend
    typedef typename backend_type::sparse_matrix_type sparse_matrix_type;
    //! sparse matrix type associated with backend (shared_ptr<> type)
    typedef typename backend_type::sparse_matrix_ptrtype sparse_matrix_ptrtype;
    //! vector type associated with backend
    typedef typename backend_type::vector_type vector_type;
    //! vector type associated with backend (shared_ptr<> type)
    typedef typename backend_type::vector_ptrtype vector_ptrtype;

    //! geometry entities type composing the mesh, here Simplex in Dimension Dim of Order 1
    typedef Simplex<Dim> convex_type;
    //typedef Hypercube<Dim,1,Dim> convex_type;
    //! mesh type
    typedef Mesh<convex_type> mesh_type;
    //! mesh shared_ptr<> type
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;

    //! function space that holds piecewise constant (\f$P_0\f$) functions (e.g. to store material properties or partitioning
    typedef FunctionSpace<mesh_type, bases<Lagrange<0,Scalar, Discontinuous> > > p0_space_type;
    //! an element type of the \f$P_0\f$ discontinuous function space
    typedef typename p0_space_type::element_type p0_element_type;

    //! the basis type of our approximation space
    typedef bases<Lagrange<Order,Scalar> > basis_type;

    //! the approximation function space type
    typedef FunctionSpace<mesh_type, basis_type> space_type;
    //! the approximation function space type (shared_ptr<> type)
    typedef boost::shared_ptr<space_type> space_ptrtype;
    //! an element type of the approximation function space
    typedef typename space_type::element_type element_type;

    //! the exporter factory type
    typedef Exporter<mesh_type> export_type;
    //! the exporter factory (shared_ptr<> type)
    typedef boost::shared_ptr<export_type> export_ptrtype;

    /**
     * Constructor
     */
    Laplacian( po::variables_map const& vm, AboutData const& about )
        :
        super( vm, about ),
        M_backend( backend_type::build( this->vm() ) ),
        meshSize( this->vm()["hsize"].template as<double>() ),
        shape( this->vm()["shape"].template as<std::string>() )
    {
    }

    void run();

    void run( const double* X, unsigned long P, double* Y, unsigned long N );

private:

    /**
     * solve the system \f$D u = F\f$
     *
     * \param in D sparse matrix
     * \param inout u solution of the system
     * \param in F vector representing the right hand side of the system
     */
    void solve( sparse_matrix_ptrtype& D, element_type& u, vector_ptrtype& F );


private:

    //! linear algebra backend
    backend_ptrtype M_backend;

    //! mesh characteristic size
    double meshSize;

    //! shape of the domain
    std::string shape;
}; // Laplacian

template<int Dim> const uint16_type Laplacian<Dim>::Order;

template<int Dim>
void
Laplacian<Dim>::run()
{
    std::cout << "------------------------------------------------------------\n";
    std::cout << "Execute Laplacian<" << Dim << ">\n";
    std::vector<double> X( 2 );
    X[0] = meshSize;

    if ( shape == "hypercube" )
        X[1] = 1;

    else // default is simplex
        X[1] = 0;

    std::vector<double> Y( 3 );
    run( X.data(), X.size(), Y.data(), Y.size() );
}
template<int Dim>
void
Laplacian<Dim>::run( const double* X, unsigned long P, double* Y, unsigned long N )
{
    if ( X[1] == 0 ) shape = "simplex";

    if ( X[1] == 1 ) shape = "hypercube";

    if ( !this->vm().count( "nochdir" ) )
        Environment::changeRepository( boost::format( "doc/tutorial/%1%/%2%-%3%/P%4%/h_%5%/" )
                                       % this->about().appName()
                                       % shape
                                       % Dim
                                       % Order
                                       % meshSize );

    /*    mesh_ptrtype mesh = createGMSHMesh( _mesh=new mesh_type,
                                        _desc=domain( _name=(boost::format( "%1%-%2%" ) % shape % Dim).str() ,
                                                      _usenames=true,
                                                      _shape=shape,
                                                      _dim=Dim,
                                                      _h=X[0] ) );*/
    mesh_ptrtype mesh = loadGMSHMesh( _mesh=new mesh_type,
                                      _filename="piece.msh",
                                      _update=MESH_CHECK|MESH_UPDATE_FACES|MESH_UPDATE_EDGES|MESH_RENUMBER );

    /**
     * The function space and some associated elements(functions) are then defined
     */
    /** \code */
    space_ptrtype Xh = space_type::New( mesh );
    element_type u( Xh, "u" );
    element_type v( Xh, "v" );
    element_type gproj( Xh, "v" );
    /** \endcode */

    /** define \f$g\f$ the expression of the exact solution and
     * \f$f\f$ the expression of the right hand side such that \f$g\f$
     * is the exact solution
     */
    /** \code */
    //# marker1 #
    value_type pi = M_PI;
    //! deduce from expression the type of g (thanks to keyword 'auto')
    auto g = cst( 0. );
    gproj = vf::project( Xh, elements( mesh ), g );

    //! deduce from expression the type of f (thanks to keyword 'auto')
    auto f = cst( 0. );
    //# endmarker1 #
    /** \endcode */

    bool weakdir = this->vm()["weakdir"].template as<int>();
    value_type penaldir = this->vm()["penaldir"].template as<double>();
    value_type nu = this->vm()["nu"].template as<double>();

    using namespace Feel::vf;

    /**
     * Construction of the right hand side. F is the vector that holds
     * the algebraic representation of the right habd side of the
     * problem
     */
    /** \code */
    //# marker2 #
    vector_ptrtype F( M_backend->newVector( Xh ) );
    form1( _test=Xh, _vector=F, _init=true ) =
        integrate( elements( mesh ), f*id( v ) )+
        integrate( markedfaces( mesh, "Mur" ), nu*gradv( gproj )*vf::N()*id( v ) );
    //# endmarker2 #
    /* if ( this->comm().size() != 1 || weakdir )
        {
            //# marker41 #
            form1( _test=Xh, _vector=F ) +=
                integrate( markedfaces(mesh,mesh->markerName("Dirichlet")), g*(-grad(v)*vf::N()+penaldir*id(v)/hFace()) );
            //# endmarker41 #
        }
        F->close();*/

    /** \endcode */

    /**
     * create the matrix that will hold the algebraic representation
     * of the left hand side
     */
    //# marker3 #
    /** \code */
    sparse_matrix_ptrtype D( M_backend->newMatrix( Xh, Xh ) );
    /** \endcode */

    //! assemble $\int_\Omega \nu \nabla u \cdot \nabla v$
    /** \code */
    form2( Xh, Xh, D, _init=true ) =
        integrate( elements( mesh ), nu*gradt( u )*trans( grad( v ) ) );
    /** \endcode */
    //# endmarker3 #

    /* if ( this->comm().size() != 1 || weakdir )
        {
            /** weak dirichlet conditions treatment for the boundaries marked 1 and 3
             * -# assemble \f$\int_{\partial \Omega} -\nabla u \cdot \mathbf{n} v\f$
             * -# assemble \f$\int_{\partial \Omega} -\nabla v \cdot \mathbf{n} u\f$
             * -# assemble \f$\int_{\partial \Omega} \frac{\gamma}{h} u v\f$
             */
    /** \code */
    //# marker10 #
    /*  form2( Xh, Xh, D ) +=
                integrate( markedfaces(mesh,mesh->markerName("Dirichlet")),
                           -(gradt(u)*vf::N())*id(v)
                           -(grad(v)*vf::N())*idt(u)
                           +penaldir*id(v)*idt(u)/hFace());
                           D->close();*/
    //# endmarker10 #
    /** \endcode */
    //     }
    /* else
            {
                /** strong(algebraic) dirichlet conditions treatment for the boundaries marked 1 and 3
                 * -# first close the matrix (the matrix must be closed first before any manipulation )
                 * -# modify the matrix by cancelling out the rows and columns of D that are associated with the Dirichlet dof
                 */
    /** \code */
    //# marker5 #
    D->close();
    form2( Xh, Xh, D ) +=
        on( markedfaces( mesh, "Poele" ), u, F, cst( 45 ) )+
        on( markedfaces( mesh, "Fenetre" ), u, F, cst( 5 ) );

    //# endmarker5 #
    /** \endcode */

    //      }
    /** \endcode */


    //! solve the system
    /** \code */
    this->solve( D, u, F );
    /** \endcode */

    //! compute the \f$L_2$ norm of the error
    /** \code */
    //# marker7 #
    double L2error2 =integrate( elements( mesh ),
                                ( idv( u )-g )*( idv( u )-g ) ).evaluate()( 0,0 );
    double L2error =   math::sqrt( L2error2 );


    LOG(INFO) << "||error||_L2=" << L2error << "\n";
    //# endmarker7 #
    /** \endcode */

    //! save the results
    /** \code */
    //! project the exact solution
    element_type e( Xh, "e" );
    e = vf::project( Xh, elements( mesh ), g );

    export_ptrtype exporter( export_type::New( this->vm(),
                             ( boost::format( "%1%-%2%-%3%" )
                               % this->about().appName()
                               % shape
                               % Dim ).str() ) );

    if ( exporter->doExport() )
    {
        LOG(INFO) << "exportResults starts\n";

        exporter->step( 0 )->setMesh( mesh );

        exporter->step( 0 )->add( "u", u );
        exporter->step( 0 )->add( "g", e );

        exporter->save();
        LOG(INFO) << "exportResults done\n";
    }

    /** \endcode */
} // Laplacian::run

//# marker6 #
template<int Dim>
void
Laplacian<Dim>::solve( sparse_matrix_ptrtype& D,
                       element_type& u,
                       vector_ptrtype& F )
{
    //! solve the system, first create a vector U of the same size as
    //! u, then call solve,
    vector_ptrtype U( M_backend->newVector( u.functionSpace() ) );
    //! call solve, the second D is the matrix which will be used to
    //! create the preconditionner
    M_backend->solve( D, D, U, F );
    //! copy U in u
    u = *U;
} // Laplacian::solve
//# endmarker6 #

/**
 * main function: entry point of the program
 */
int
main( int argc, char** argv )
{
    Environment env( argc, argv );
    /**
     * create an application
     */
    /** \code */
    Application app( argc, argv, makeAbout(), makeOptions() );

    if ( app.vm().count( "help" ) )
    {
        std::cout << app.optionsDescription() << "\n";
        return 0;
    }

    /** \endcode */

    /**
     * register the simgets
     */
    /** \code */
    app.add( new Laplacian<1>( app.vm(), app.about() ) );
    app.add( new Laplacian<2>( app.vm(), app.about() ) );
    app.add( new Laplacian<3>( app.vm(), app.about() ) );
    /** \endcode */

    /**
     * run the application
     */
    /** \code */
    app.run();
    /** \endcode */
}





