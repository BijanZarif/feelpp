/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=cpp:et:sw=4:ts=4:sts=4*/


#include <feel/feelmodels/solid/solidmecbase.hpp>

//#include <feel/feeldiscr/pchv.hpp>

namespace Feel
{
namespace FeelModels
{

//-------------------------------------------------------------------------------------//

SOLIDMECHANICSBASE_CLASS_TEMPLATE_DECLARATIONS
void
SOLIDMECHANICSBASE_CLASS_TEMPLATE_TYPE::updateLinearPDE(const vector_ptrtype& X,
                                                                         sparse_matrix_ptrtype& A ,
                                                                         vector_ptrtype& F,
                                                                         bool _buildCstPart,
                                                                         sparse_matrix_ptrtype& A_extended, bool _BuildExtendedPart,
                                                                         bool _doClose, bool _doBCStrongDirichlet ) const
{
    if (M_pdeType=="Elasticity")
    {
        //this->updateLinearElasticity(X,A,F);
        this->updateLinearElasticityGeneralisedAlpha(X,A,F,
                                                     _buildCstPart,
                                                     _doClose, _doBCStrongDirichlet );
    }
    else if (M_pdeType=="Generalised-String")
    {
        this->updateLinearGeneralisedStringGeneralisedAlpha(X,A,F,
                                                            _buildCstPart,
                                                            _doClose, _doBCStrongDirichlet );
    }
}

//-------------------------------------------------------------------------------------//

SOLIDMECHANICSBASE_CLASS_TEMPLATE_DECLARATIONS
void
SOLIDMECHANICSBASE_CLASS_TEMPLATE_TYPE::updateLinearElasticityGeneralisedAlpha(const vector_ptrtype& X,
                                                                                                sparse_matrix_ptrtype& A,
                                                                                                vector_ptrtype& F,
                                                                                                bool _buildCstPart,
                                                                                                bool _doClose, bool _doBCStrongDirichlet ) const
{
#if defined(FEELMODELS_SOLID_BUILD_LINEAR_CODE)

    using namespace Feel::vf;

    this->log("SolidMechanics","updateLinearElasticityGeneralisedAlpha", "start" );
    this->timerTool("Solve").start();
    //boost::timer thetimer;

    //---------------------------------------------------------------------------------------//

    bool BuildNonCstPart = !_buildCstPart;
    bool BuildCstPart = _buildCstPart;
    bool BuildNonCstPart_TransientForm2Term = BuildNonCstPart;
    bool BuildNonCstPart_TransientForm1Term = BuildNonCstPart;
    bool BuildNonCstPart_SourceTerm = BuildNonCstPart;
    bool BuildNonCstPart_BoundaryNeumannTerm = BuildNonCstPart;
    if (this->useFSISemiImplicitScheme())
    {
        BuildNonCstPart_TransientForm2Term = BuildCstPart;
        BuildNonCstPart_TransientForm1Term=BuildCstPart;
        BuildNonCstPart_SourceTerm=BuildCstPart;
        BuildNonCstPart_BoundaryNeumannTerm=BuildCstPart;
    }
    if (M_newmark_displ_struct->strategy()==TS_STRATEGY_DT_CONSTANT)
    {
        BuildNonCstPart_TransientForm2Term = BuildCstPart;
    }
    //---------------------------------------------------------------------------------------//

    auto mesh = M_Xh->mesh();
    auto Xh = M_Xh;

    size_type rowStartInMatrix = this->rowStartInMatrix();
    size_type colStartInMatrix = this->colStartInMatrix();
    size_type rowStartInVector = this->rowStartInVector();
    auto u = Xh->element("u");//u = *X;
    auto v = Xh->element("v");
    for ( size_type k=0;k<M_Xh->nLocalDofWithGhost();++k )
        u(k) = X->operator()(rowStartInVector+k);

    //auto buzz1 = M_newmark_displ_struct->previousUnknown();
    //---------------------------------------------------------------------------------------//
    auto const& coeffLame1 = this->mechanicalProperties()->fieldCoeffLame1();
    auto const& coeffLame2 = this->mechanicalProperties()->fieldCoeffLame2();
    auto const& rho = this->mechanicalProperties()->fieldRho();
    // Identity Matrix
    auto const Id = eye<nDim,nDim>();
    // deformation tensor
    auto epst = sym(gradt(u));//0.5*(gradt(u)+trans(gradt(u)));
    auto eps = sym(grad(u));
    //---------------------------------------------------------------------------------------//
    // stress tensor
#if 0
    //#if (SOLIDMECHANICS_DIM==2) // cas plan
    double lll = 2*idv(coeffLame1)*idv(coeffLame2)/(idv(coeffLame1)+2*idv(coeffLame2));
    auto sigmat = lll*trace(epst)*Id + 2*idv(coeffLame2)*epst;
    auto sigmaold = lll*trace(epsold)*Id + 2*idv(coeffLame2)*epsold;
    //#endif
#endif
    //#if (SOLIDMECHANICS_DIM==3)  // cas 3d
    auto sigmat = idv(coeffLame1)*trace(epst)*Id + 2*idv(coeffLame2)*epst;
    //auto sigmaold = idv(coeffLame1)*trace(epsold)*Id + 2*idv(coeffLame2)*epsold;
    //#endif
    //---------------------------------------------------------------------------------------//

    double rho_s=0.8;
    double alpha_f=M_genAlpha_alpha_f;
    double alpha_m=M_genAlpha_alpha_m;
    double gamma=0.5+alpha_m-alpha_f;
    double beta=0.25*(1+alpha_m-alpha_f)*(1+alpha_m-alpha_f);

    //---------------------------------------------------------------------------------------//
    // internal force term
    if (BuildCstPart)
    {
        if ( !this->useDisplacementPressureFormulation() )
        {
            form2( _test=Xh, _trial=Xh, _matrix=A )  +=
                integrate (_range=elements(mesh),
                           _expr= alpha_f*trace( sigmat*trans(grad(v))),
                           _geomap=this->geomap() );
        }
        else
        {
            form2( _test=Xh, _trial=Xh, _matrix=A )  +=
                integrate (_range=elements(mesh),
                           _expr= 2*idv(coeffLame2)*inner(epst,grad(v)),
                           _geomap=this->geomap() );

            auto p = M_XhPressure->element();//*M_fieldPressure;
            size_type startDofIndexPressure = this->startDofIndexFieldsInMatrix().find("pressure")->second;
            form2( _test=Xh, _trial=M_XhPressure, _matrix=A,
                   _rowstart=rowStartInMatrix,
                   _colstart=colStartInMatrix+startDofIndexPressure ) +=
                integrate (_range=elements(mesh),
                           _expr= idt(p)*div(v),
                           _geomap=this->geomap() );
            form2( _test=M_XhPressure, _trial=Xh, _matrix=A,
                   _rowstart=rowStartInMatrix+startDofIndexPressure,
                   _colstart=colStartInMatrix ) +=
                integrate(_range=elements(mesh),
                          _expr= id(p)*divt(u),
                          _geomap=this->geomap() );
            form2( _test=M_XhPressure, _trial=M_XhPressure, _matrix=A,
                   _rowstart=rowStartInMatrix+startDofIndexPressure,
                   _colstart=colStartInMatrix+startDofIndexPressure ) +=
                integrate(_range=elements(mesh),
                          _expr= -(cst(1.)/idv(coeffLame1))*idt(p)*id(p),
                          _geomap=this->geomap() );
        }
    }
    //---------------------------------------------------------------------------------------//
    // discretisation acceleration term
    if (!this->isStationary() && BuildNonCstPart_TransientForm2Term)
    {
        form2( _test=Xh, _trial=Xh, _matrix=A )  +=
            integrate( _range=elements(mesh),
                       _expr= M_newmark_displ_struct->polySecondDerivCoefficient()*idv(rho)*inner(idt(u),id(v)),
                       _geomap=this->geomap() );
    }
    //---------------------------------------------------------------------------------------//
    // discretisation acceleration term
    if (!this->isStationary() && BuildNonCstPart_TransientForm1Term)
    {
        form1( _test=Xh, _vector=F ) +=
            integrate( _range=elements(mesh),
                       _expr= idv(rho)*trans(idv(M_newmark_displ_struct->polyDeriv()))*id(v),
                       _geomap=this->geomap() );
    }
    //---------------------------------------------------------------------------------------//
    // source term
    if ( BuildNonCstPart_SourceTerm )
    {
        this->updateSourceTermLinearPDE( F );
    }
    //---------------------------------------------------------------------------------------//
    // neumann boundary condition
    if (BuildNonCstPart_BoundaryNeumannTerm)
    {
        this->updateBCNeumannLinearPDE( F );
    }
    //---------------------------------------------------------------------------------------//

    if (this->markerNameFSI().size()>0)
    {
        // neumann boundary condition with normal stress (fsi boundary condition)
        if (BuildNonCstPart)
        {
            form1( _test=Xh, _vector=F) +=
                integrate( _range=markedfaces(mesh,this->markerNameFSI()),
                           _expr= -alpha_f*trans(idv(*M_normalStressFromFluid))*id(v),
                           _geomap=this->geomap() );
        }

        if ( this->couplingFSIcondition() == "robin-robin" || this->couplingFSIcondition() == "robin-robin-genuine" ||
             this->couplingFSIcondition() == "nitsche" )
        {

            double gammaRobinFSI = this->gammaNitschFSI();
            double muFluid = this->muFluidFSI();
#if 0
            // integrate on ref with variables change
            auto Fa = eye<nDim,nDim>() + gradv(this->timeStepNewmark()->previousUnknown());
            auto J = det(Fa);
            auto B = inv(Fa);
            auto variablechange = J*norm2( B*N() );
            if (BuildNonCstPart )
            {
                form2( _test=Xh, _trial=Xh, _matrix=A )  +=
                    integrate( _range=markedfaces(mesh,this->markerNameFSI()),
                               _expr= variablechange*gammaRobinFSI*muFluid*this->timeStepNewmark()->polyFirstDerivCoefficient()*inner(idt(u),id(v))/hFace(),
                               _geomap=this->geomap() );
                auto robinFSIRhs = idv(this->timeStepNewmark()->polyFirstDeriv() ) + idv(this->velocityInterfaceFromFluid());
                form1( _test=Xh, _vector=F) +=
                    integrate( _range=markedfaces(mesh,this->markerNameFSI()),
                               _expr= variablechange*gammaRobinFSI*muFluid*inner( robinFSIRhs, id(v))/hFace(),
                               _geomap=this->geomap() );

            }
#else
            if (BuildNonCstPart )//BuildCstPart)
            {
                MeshMover<mesh_type> mymesh_mover;
                mesh_ptrtype mymesh = this->mesh();
                mymesh_mover.apply( mymesh, this->timeStepNewmark()->previousUnknown() );

                form2( _test=Xh, _trial=Xh, _matrix=A )  +=
                    integrate( _range=markedfaces(mesh,this->markerNameFSI()),
                               _expr= gammaRobinFSI*muFluid*this->timeStepNewmark()->polyFirstDerivCoefficient()*inner(idt(u),id(v))/hFace(),
                               _geomap=this->geomap() );

                auto robinFSIRhs = idv(this->timeStepNewmark()->polyFirstDeriv() ) + idv(this->velocityInterfaceFromFluid());
                form1( _test=Xh, _vector=F) +=
                    integrate( _range=markedfaces(mesh,this->markerNameFSI()),
                               _expr= gammaRobinFSI*muFluid*inner( robinFSIRhs, id(v))/hFace(),
                               _geomap=this->geomap() );

                auto dispInv = this->fieldDisplacement().functionSpace()->element(-idv(this->timeStepNewmark()->previousUnknown()));
                mymesh_mover.apply( mymesh, dispInv );
            }
#endif

        }
    }

    //---------------------------------------------------------------------------------------//

    // robin condition (used in fsi blood flow as external tissue)
    if ( !this->markerRobinBC().empty() && !BuildCstPart )
    {
        this->updateBCRobinLinearPDE( A, F );
    }

    //---------------------------------------------------------------------------------------//

    if ( this->hasMarkerDirichletBCelimination() && BuildNonCstPart && _doBCStrongDirichlet)
    {
        this->updateBCDirichletStrongLinearPDE( A,F );
    }

    //---------------------------------------------------------------------------------------//
    double timeElapsed = this->timerTool("Solve").stop("createMesh");
    this->log("SolidMechanics","updateLinearElasticityGeneralisedAlpha",
              (boost::format("finish in %1% s") % timeElapsed).str() );

#endif // FEELMODELS_SOLID_BUILD_LINEAR_CODE
} // updateLinearElasticityGeneralisedAlpha




} // FeelModels

} // Feel


