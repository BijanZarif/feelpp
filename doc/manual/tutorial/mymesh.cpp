/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2008-02-06

  Copyright (C) 2008-2009 Université Joseph Fourier (Grenoble I)


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
   \file mymesh.cpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2008-02-06
 */
#include <feel/feel.hpp>

using namespace Feel;

inline
Feel::po::options_description
makeOptions()
{
    Feel::po::options_description mymeshoptions( "MyMesh options" );
    mymeshoptions.add_options()
    ( "hsize", Feel::po::value<double>()->default_value( 0.1 ), "mesh size" )
    ( "shape", Feel::po::value<std::string>()->default_value( "hypercube" ), "shape of the domain (either simplex or hypercube)" )
    ;

    // return the options mymeshoptions and the feel_options defined
    // internally by Feel
    return mymeshoptions.add( Feel::feel_options() );
}
inline
Feel::AboutData
makeAbout()
{
    Feel::AboutData about( "mymesh" ,
                           "mymesh" ,
                           "0.2",
                           "my first Feel application",
                           Feel::AboutData::License_GPL,
                           "Copyright (c) 2008-2010 Universite Joseph Fourier" );

    about.addAuthor( "Christophe Prud'homme",
                     "developer",
                     "christophe.prudhomme@feelpp.org", "" );
    return about;
}

template<int Dim>
class MyMesh: public Feel::Simget
{
public:

    //# marker1 #
    typedef Simplex<Dim> convex_type;
    //typedef Hypercube<Dim, 1,Dim> convex_type;
    //# endmarker1 #

    //# marker2 #
    typedef Mesh<convex_type > mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;
    //# endmarker2 #

    //# marker61 #
    /* export */
    typedef Exporter<mesh_type> export_type;
    typedef boost::shared_ptr<export_type> export_ptrtype;
    //# endmarker61 #

    /**
     * constructor about data and options description
     */
    MyMesh();

    void run();

private:
    double meshSize;
    std::string shape;
};

template<int Dim>
MyMesh<Dim>::MyMesh()
    :
    Simget(),
    meshSize( this->vm()["hsize"].template as<double>() ),
    shape( this->vm()["shape"].template as<std::string>() )
{}


template<int Dim>
void
MyMesh<Dim>::run()
{
    Environment::changeRepository( boost::format( "doc/manual/tutorial/%1%/%2%-%3%/h_%4%" )
                                   % this->about().appName()
                                   % shape
                                   % Dim
                                   % meshSize );

    //# marker4 #
    LOG(INFO) << "Generating mesh using gmsh...\n";
    auto mesh = createGMSHMesh( _mesh=new mesh_type,
                                _desc=domain( _name=( boost::format( "%1%-%2%" ) % shape % Dim ).str() ,
                                              _shape=shape,
                                              _dim=Dim,
                                              _h=meshSize ) );
    //# endmarker4 #

    LOG(INFO) << "Local number of elements: " << mesh->numElements() << "\n";
    LOG(INFO) << "Number of global elements: " << mesh->numGlobalElements() << "\n";

    //# marker62 #

    export_ptrtype exporter( export_type::New() );
    LOG(INFO) << "Exporting mesh\n";
    exporter->step( 0 )->setMesh( mesh );
    LOG(INFO) << "Exporting regions\n";
    exporter->step( 0 )->addRegions();
    LOG(INFO) << "Saving...\n";
    exporter->save();

    //# endmarker62 #
}

//
// main function: entry point of the program
//
int main( int argc, char** argv )
{
    /**
     * Initialize Feel++ Environment
     */
    Environment env( _argc=argc, _argv=argv,
                     _desc=makeOptions(),
                     _about=about(_name="mymesh",
                                  _author="Christophe Prud'homme",
                                  _email="christophe.prudhomme@feelpp.org") );

    Application app;


    if ( Environment::numberOfProcessors() == 1 )
        app.add( new MyMesh<1>() );
    app.add( new MyMesh<2>() );
    app.add( new MyMesh<3>() );

    app.run();
}
