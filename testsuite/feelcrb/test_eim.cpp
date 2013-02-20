/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t  -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2012-05-23

  Copyright (C) 2012 Université Joseph Fourier (Grenoble I)

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
   \file test_eim.cpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2012-05-23
 */
#define USE_BOOST_TEST 1
#define BOOST_TEST_MODULE eim testsuite

#include <testsuite/testsuite.hpp>

#include <boost/timer.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <feel/feelcore/environment.hpp>
#include <feel/feelcore/application.hpp>
#include <feel/options.hpp>
#include <feel/feelalg/backend.hpp>
#include <feel/feeldiscr/functionspace.hpp>
#include <feel/feelfilters/gmsh.hpp>
#include <feel/feelcrb/eim.hpp>

#define FEELAPP( argc, argv, about, options )                           \
    Feel::Application app( argc, argv, about, options );                \
    if ( app.vm().count( "help" ) )                                     \
    {                                                                   \
        std::cout << app.optionsDescription() << "\n";                  \
    }


namespace Feel
{
inline
po::options_description
makeOptions()
{
    po::options_description simgetoptions( "test_eim options" );
    simgetoptions.add_options()
    ( "hsize", po::value<double>()->default_value( 0.5 ), "mesh size" )
    ;
    return simgetoptions.add( eimOptions() ).add( Feel::feel_options() );
}


inline
AboutData
makeAbout()
{
    AboutData about( "test_simget" ,
                     "test_simget" ,
                     "0.1",
                     "SimGet tests",
                     Feel::AboutData::License_GPL,
                     "Copyright (c) 2012 Université Joseph Fourier" );

    about.addAuthor( "Christophe Prud'homme", "developer", "christophe.prudhomme@feelpp.org", "" );
    return about;

}

/**
 *
 */
class model:
    public Simget,
    public boost::enable_shared_from_this<model>
{
public:
    typedef Mesh<Simplex<2> > mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;
    typedef FunctionSpace<mesh_type,bases<Lagrange<1> > > space_type;
    typedef boost::shared_ptr<space_type> space_ptrtype;
    typedef space_type functionspace_type;
    typedef space_ptrtype functionspace_ptrtype;
    typedef space_type::element_type element_type;


    typedef ParameterSpace<1> parameterspace_type;
    typedef boost::shared_ptr<parameterspace_type> parameterspace_ptrtype;
    typedef parameterspace_type::element_type parameter_type;
    typedef parameterspace_type::element_ptrtype parameter_ptrtype;
    typedef parameterspace_type::sampling_type sampling_type;
    typedef parameterspace_type::sampling_ptrtype sampling_ptrtype;

    typedef EIMFunctionBase<space_type, space_type, parameterspace_type> fun_type;
    typedef boost::shared_ptr<fun_type> fun_ptrtype;
    typedef std::vector<fun_ptrtype> funs_type;

    model()
        :
        Simget(),
        meshSize( this->vm()["hsize"].as<double>() )
        {

            mesh = createGMSHMesh( _mesh=new mesh_type,
                                   _desc=domain( _name=( boost::format( "%1%-%2%" ) % "hypercube" % 2 ).str() ,
                                                 _usenames=true,
                                                 _shape="hypercube",
                                                 _dim=2,
                                                 _h=0.025 ) );

            Xh =  space_type::New( mesh );
            BOOST_CHECK( Xh );
            u = Xh->element();
            Dmu = parameterspace_type::New();
            BOOST_CHECK( Dmu );

            parameter_type mu_min( Dmu );
            mu_min << 0.1;
            Dmu->setMin( mu_min );
            parameter_type mu_max( Dmu );
            mu_max << 1;
            Dmu->setMax( mu_max );
            mu = Dmu->element();
            BOOST_CHECK_EQUAL( mu.parameterSpace(), Dmu );

            auto Pset = Dmu->sampling();
            int sampling_size = this->vm()["eim.sampling-size"].as<int>();
            Pset->randomize( sampling_size );

            BOOST_TEST_MESSAGE( "Allocation done" );
            BOOST_TEST_MESSAGE( "pushing function to be empirically interpolated" );

            using namespace vf;
            //auto p = this->shared_from_this();
            //BOOST_CHECK( p );
            //BOOST_TEST_MESSAGE( "shared from this" );
#if 1
            LOG(INFO) << "=== sin(cst_ref(mu(0))*idv(u)*idv(u)) === \n";
            auto e = eim( _model=this,
                          _options=this->vm(),
                          _element=u,
                          _space=this->functionSpace(),
                          _parameter=mu,
                          _expr=sin(cst_ref(mu(0))*idv(u)*idv(u)),
                          _sampling=Pset,
                          _name="q1" );
            BOOST_TEST_MESSAGE( "create e done" );
            BOOST_CHECK( e );
            M_funs.push_back( e );
#endif
            LOG(INFO) << "=== mu(0) === \n";
            auto e1 = eim( _model=eim_no_solve(this),
                           _options=this->vm(),
                           _element=u,
                           _space=this->functionSpace(),
                           _parameter=mu,
                           _expr=cst_ref(mu(0)),
                           _sampling=Pset,
                           _name="mu0" );
            BOOST_TEST_MESSAGE( "create e1 done" );
            M_funs.push_back( e1 );
            BOOST_CHECK_EQUAL( e1->mMax(), 1 );

            LOG(INFO) << "=== mu(0) x === \n";
            auto e2 = eim( _model=eim_no_solve(this),
                           _options=this->vm(),
                           _element=u,
                           _space=this->functionSpace(),
                           _parameter=mu,
                           _expr=cst_ref(mu(0))*Px(),
                           _sampling=Pset,
                           _name="mu0x" );
            BOOST_TEST_MESSAGE( "create e2 done" );
            M_funs.push_back( e2 );
            LOG(INFO) << "=== sin(2 pi mu(0) x) === \n";
            auto e3 = eim( _model=eim_no_solve(this),
                           _options=this->vm(),
                           _element=u,
                           _space=this->functionSpace(),
                           _parameter=mu,
                           _expr=sin(2*constants::pi<double>()*cst_ref(mu(0))*Px()),
                           _sampling=Pset,
                           _name="sin2pimu0x" );
            BOOST_TEST_MESSAGE( "create e3 done" );
            M_funs.push_back( e3 );
            LOG(INFO) << "=== exp(-((Px()-0.5)*(Px()-0.5)+(Py()-0.5)*(Py()-0.5))/(2*mu(0)*mu(0))), === \n";
            auto e5 = eim( _model=eim_no_solve(this),
                           _options=this->vm(),
                           _element=u,
                           _space=this->functionSpace(),
                           _parameter=mu,
                           _expr=exp(-((Px()-0.5)*(Px()-0.5)+(Py()-0.5)*(Py()-0.5))/(2*cst_ref(mu(0))*cst_ref(mu(0)))),
                           _sampling=Pset,
                           _name="q2" );
            BOOST_TEST_MESSAGE( "create e5 done" );
            M_funs.push_back( e5 );

            BOOST_TEST_MESSAGE( "function to apply eim pushed" );

        }
    //! return the parameter space
    parameterspace_ptrtype parameterSpace() const
        {
            return Dmu;
        }
    std::string modelName() const { return std::string("test_eim_model1" );}

    space_ptrtype functionSpace() { return Xh; }

    element_type solve( parameter_type const& mu )
        {
            auto a = form2( _test=Xh, _trial=Xh  );
            auto rhs = form1( _test=Xh );
            auto v = Xh->element();
            auto w = Xh->element();
            rhs = integrate( _range=elements( mesh ), _expr=id(v) );
            a = integrate( _range=elements( mesh ), _expr=mu(0)*gradt( w )*trans( grad( v ) ) + idt(w)*id(v) );
            a+=on( _range=boundaryfaces(mesh),
                   _element=w, _rhs=rhs, _expr=cst(0.) );
            a.solve( _solution=w, _rhs=rhs );
            return w;
        }
    void run()
        {
            auto e = exporter( _mesh=mesh, _name=this->about().appName() );
            auto S = Dmu->sampling();
            S->logEquidistribute(10);
            BOOST_FOREACH( auto fun, M_funs )
            {
                BOOST_FOREACH( auto p, *S )
                {
                    LOG(INFO) << "evaluate model at p = " << p << "\n";
                    auto v = fun->operator()( p );
                    LOG(INFO) << "evaluate eim interpolant at p = " << p << "\n";
                    auto w = fun->interpolant( p );

                    e->add( (boost::format( "%1%(%2%)" ) % fun->name() % p(0) ).str(), v );
                    e->add( (boost::format( "%1%-eim(%2%)" ) % fun->name() % p(0) ).str(), w );

                }
            }
            e->save();

        }
    void run( const double*, long unsigned int, double*, long unsigned int ) {}
private:
    double meshSize;
    mesh_ptrtype mesh;
    space_ptrtype Xh;
    element_type u;
    parameterspace_ptrtype Dmu;
    parameter_type mu;

    funs_type M_funs;
};


//model circle
class model_circle:
    public Simget,
    public boost::enable_shared_from_this<model_circle>
{
public:
    typedef Mesh<Simplex<2> > mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;
    typedef FunctionSpace<mesh_type,bases<Lagrange<1> > > space_type;
    typedef boost::shared_ptr<space_type> space_ptrtype;
    typedef space_type functionspace_type;
    typedef space_ptrtype functionspace_ptrtype;
    typedef space_type::element_type element_type;

    typedef ParameterSpace<4> parameterspace_type;
    typedef boost::shared_ptr<parameterspace_type> parameterspace_ptrtype;
    typedef parameterspace_type::element_type parameter_type;
    typedef parameterspace_type::element_ptrtype parameter_ptrtype;
    typedef parameterspace_type::sampling_type sampling_type;
    typedef parameterspace_type::sampling_ptrtype sampling_ptrtype;

    typedef EIMFunctionBase<space_type, space_type, parameterspace_type> fun_type;
    typedef boost::shared_ptr<fun_type> fun_ptrtype;
    typedef std::vector<fun_ptrtype> funs_type;

    model_circle()
        :
        Simget(),
        meshSize( this->vm()["hsize"].as<double>() )
        {

            mesh = createGMSHMesh( _mesh=new mesh_type,
                                   _desc=domain( _name=( boost::format( "%1%-%2%" ) % "hypercube" % 2 ).str() ,
                                                 _usenames=true,
                                                 _shape="hypercube",
                                                 _dim=2,
                                                 _h=0.1 ) );


            Xh =  space_type::New( mesh );
            BOOST_CHECK( Xh );
            u = Xh->element();
            Dmu = parameterspace_type::New();
            BOOST_CHECK( Dmu );

            //function :
            // \alpha ( x - cx )^2 + \beta ( y - cy )^2
            parameter_type mu_min( Dmu );
            mu_min << /*alpha*/0.01, /*beta*/ 0.01, /*cx*/ 1e-8, /*cy*/ 1e-8;
            Dmu->setMin( mu_min );
            parameter_type mu_max( Dmu );
            mu_max << /*alpha*/2, /*beta*/ 2, /*cx*/ 1, /*cy*/ 1;
            Dmu->setMax( mu_max );
            mu = Dmu->element();
            BOOST_CHECK_EQUAL( mu.parameterSpace(), Dmu );

            auto Pset = Dmu->sampling();
            int sampling_size = this->vm()["eim.sampling-size"].as<int>();
            Pset->randomize( sampling_size );

            BOOST_TEST_MESSAGE( "Allocation done" );
            BOOST_TEST_MESSAGE( "pushing function to be empirically interpolated" );

            LOG(INFO) << "===  cst_ref(mu(0)) *( Px() - cst_ref(mu(2)) )*( Px() - cst_ref(mu(2)) )+cst_ref(mu(1)) *( Py() - cst_ref(mu(3)) )*( Py() - cst_ref(mu(3)) ) === \n";
            using namespace vf;
            //auto p = this->shared_from_this();
            //BOOST_CHECK( p );
            //BOOST_TEST_MESSAGE( "shared from this" );
            auto e = eim( _model=this,
                          _options=this->vm(),
                          _element=u,
                          _space=this->functionSpace(),
                          _parameter=mu,
                          _expr= cst_ref(mu(0)) *( Px() - cst_ref(mu(2)) )*( Px() - cst_ref(mu(2)) )+cst_ref(mu(1)) *( Py() - cst_ref(mu(3)) )*( Py() - cst_ref(mu(3)) ),
                          _sampling=Pset,
                          _name="q_1");
            BOOST_TEST_MESSAGE( "create eim" );
            BOOST_CHECK( e );

            M_funs.push_back( e );
            BOOST_TEST_MESSAGE( "function to apply eim pushed" );

        }
    std::string modelName() const { return std::string("test_eim_model2" );}
    //! return the parameter space
    parameterspace_ptrtype parameterSpace() const
    {
        return Dmu;
    }

    space_ptrtype functionSpace() { return Xh; }

    element_type solve( parameter_type const& mu )
        {
            auto w = Xh->element();
            return w;
        }
    void run()
        {
            auto e = exporter( _mesh=mesh, _name="model_circle" );
            auto S = Dmu->sampling();
            S->logEquidistribute(10);
            BOOST_FOREACH( auto fun, M_funs )
            {
                BOOST_FOREACH( auto p, *S )
                {
                    LOG(INFO) << "evaluate model at p = " << p << "\n";
                    auto v = fun->operator()( p );
                    LOG(INFO) << "evaluate eim interpolant at p = " << p << "\n";
                    auto w = fun->interpolant( p );

                    e->add( (boost::format( "model2-%1%(%2%)" ) % fun->name() % p(0) ).str(), v );
                    e->add( (boost::format( "model2-%1%-eim(%2%-%3%-%4%-%5%)" ) % fun->name() % p(0) %p(1) %p(2) %p(3) ).str(), w );

                }
            }
            e->save();

        }
    void run( const double*, long unsigned int, double*, long unsigned int ) {}
private:
    double meshSize;
    mesh_ptrtype mesh;
    space_ptrtype Xh;
    element_type u;
    parameterspace_ptrtype Dmu;
    parameter_type mu;

    funs_type M_funs;
};

} // Feel

using namespace Feel;

FEELPP_ENVIRONMENT_WITH_OPTIONS( makeAbout(), makeOptions() )

BOOST_AUTO_TEST_SUITE( eimsuite )

BOOST_AUTO_TEST_CASE( test_eim1 )
{
    BOOST_TEST_MESSAGE( "test_eim1 done" );

    Application app;
    app.add( new model );
    app.run();

    BOOST_TEST_MESSAGE( "test_eim1 done" );

}
BOOST_AUTO_TEST_CASE( test_eim2 )
{
    BOOST_TEST_MESSAGE( "test_eim2 done" );

    Application app;
    app.add( new model_circle );
    app.run();

    BOOST_TEST_MESSAGE( "test_eim2 done" );

}

BOOST_AUTO_TEST_SUITE_END()


