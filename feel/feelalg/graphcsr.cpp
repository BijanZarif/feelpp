/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=cpp:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
       Date: 2007-10-29

  Copyright (C) 2007-2011 Universite Joseph Fourier (Grenoble I)

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
   \file graphcsr.cpp
   \author Christophe Prud'homme <christophe.prudhomme@feelpp.org>
   \date 2007-10-29
 */
#include <boost/timer.hpp>

#include <feel/feelcore/application.hpp>
#include <feel/feelalg/graphcsr.hpp>

namespace Feel
{
GraphCSR::GraphCSR( size_type n,
                    size_type first_row_entry_on_proc,
                    size_type last_row_entry_on_proc,
                    size_type first_col_entry_on_proc,
                    size_type last_col_entry_on_proc,
                    WorldComm const& worldcomm )
    :
    M_is_closed( false ),
    M_worldComm( worldcomm ),
    M_first_row_entry_on_proc( first_row_entry_on_proc ),
    M_last_row_entry_on_proc( last_row_entry_on_proc ),
    M_first_col_entry_on_proc( first_col_entry_on_proc ),
    M_last_col_entry_on_proc( last_col_entry_on_proc ),
    M_max_nnz( 0 ),
    M_n_total_nz( n, 0 ),
    M_n_nz( n, 0 ),
    M_n_oz( n, 0 ),
    M_storage()
{
    //std::cout << "creating graph " << this << "\n";
}

GraphCSR::GraphCSR( GraphCSR const & g )
    :
    M_is_closed( g.M_is_closed ),
    M_worldComm( g.M_worldComm ),
    M_first_row_entry_on_proc( g.M_first_row_entry_on_proc ),
    M_last_row_entry_on_proc( g.M_last_row_entry_on_proc ),
    M_first_col_entry_on_proc( g.M_first_col_entry_on_proc ),
    M_last_col_entry_on_proc( g.M_last_col_entry_on_proc ),
    M_max_nnz( g.M_max_nnz ),
    M_n_total_nz( g.M_n_total_nz ),
    M_n_nz( g.M_n_nz ),
    M_n_oz( g.M_n_oz ),
    M_storage( g.M_storage ),
    M_graphT( g.M_graphT )
{
}

GraphCSR::~GraphCSR()
{
    M_storage.clear();
}

GraphCSR&
GraphCSR::operator=( GraphCSR const& g )
{
    if ( this != &g )
    {
        M_first_row_entry_on_proc = g.M_first_row_entry_on_proc;
        M_last_row_entry_on_proc = g.M_last_row_entry_on_proc;
        M_first_col_entry_on_proc = g.M_first_col_entry_on_proc;
        M_last_col_entry_on_proc = g.M_last_col_entry_on_proc;
        M_max_nnz = g.M_max_nnz;
        M_n_total_nz= g.M_n_total_nz;
        M_n_nz = g.M_n_nz;
        M_n_oz = g.M_n_oz;
        M_storage = g.M_storage;
        M_graphT = g.M_graphT;
        M_is_closed = g.M_is_closed;
        M_worldComm = g.M_worldComm;
    }

    return *this;
}


void
GraphCSR::zero()
{
    //auto nbDof = M_last_row_entry_on_proc-M_first_row_entry_on_proc+1;
    for ( size_type i=M_first_row_entry_on_proc ; i<=M_last_row_entry_on_proc ; ++i )
    {
        row_type& row = this->row( i );
        row.get<0>() = this->worldComm().globalRank();//proc
        row.get<1>() = i-M_first_row_entry_on_proc; //local index
        row.get<2>().clear(); //all is zero
    }
}

GraphCSR::self_ptrtype
GraphCSR::transpose()
{

    if ( M_graphT ) return M_graphT;

    this->close();
    M_graphT = self_ptrtype( new self_type( M_n_total_nz.size(),
                                            M_first_col_entry_on_proc,
                                            M_last_col_entry_on_proc,
                                            M_first_row_entry_on_proc,
                                            M_last_row_entry_on_proc,
                                            this->worldComm() ) );

    for ( auto it = M_storage.begin(), en = M_storage.end() ; it != en; ++it )
    {
        // Get the row of the sparsity pattern
        row_type const& irow = it->second;

        if ( ( int )boost::get<0>( irow ) == this->worldComm().globalRank() )
        {
            // num line
            size_type globalindex = it->first;

            //size_type localindex = boost::get<1>( irow );
            for ( auto colit = boost::get<2>( irow ).begin(), colen=boost::get<2>( irow ).end() ; colit!=colen ; ++colit )
            {
                self_type::row_type& row = M_graphT->row( *colit );
                row.get<0>()=irow.get<0>();
                // Warning : wrong in parallele
                row.get<1>()=*colit;//globalindex;
                row.get<2>().insert( globalindex );
            }
        }
    }

    M_graphT->close();

    return M_graphT;
}

GraphCSR::self_ptrtype
GraphCSR::transpose(DataMap const& dm)
{

    if ( M_graphT ) return M_graphT;

    this->close();
    M_graphT = self_ptrtype( new self_type( M_n_total_nz.size(),
                                            M_first_col_entry_on_proc,
                                            M_last_col_entry_on_proc,
                                            M_first_row_entry_on_proc,
                                            M_last_row_entry_on_proc,
                                            dm.worldComm() ) );

    for ( auto it = M_storage.begin(), en = M_storage.end() ; it != en; ++it )
    {
        // Get the row of the sparsity pattern
        row_type const& irow = it->second;

        // num line
        size_type globalindex = it->first;

        for ( auto colit = boost::get<2>( irow ).begin(), colen=boost::get<2>( irow ).end() ; colit!=colen ; ++colit )
            {
                if ( *colit >= M_graphT->firstRowEntryOnProc() && *colit<=M_graphT->lastRowEntryOnProc() )
                    {
                        self_type::row_type& row = M_graphT->row( *colit );
                        row.get<0>()=this->worldComm().globalRank();
                        row.get<1>()=*colit-dm.firstDofGlobalCluster();
                        row.get<2>().insert( globalindex );
                    }
                else
                    {
                        const int realproc = dm.procOnGlobalCluster(*colit);
                        self_type::row_type& row = M_graphT->row( *colit );
                        row.get<0>()=realproc;
                        row.get<1>()=*colit-dm.firstDofGlobalCluster(realproc);
                        row.get<2>().insert( globalindex );
                    }
            }
    }
    M_graphT->close();

    return M_graphT;
}

void
GraphCSR::addMissingZeroEntriesDiagonal()
{
    size_type m = this->lastRowEntryOnProc()-this->firstRowEntryOnProc()+1;
    size_type n = this->lastColEntryOnProc()-this->firstColEntryOnProc()+1;

    for ( size_type i = this->firstRowEntryOnProc() ; i< this->firstRowEntryOnProc()+std::min( m,n ) ; ++i )
    {
        if ( this->storage().find( i )!=this->end() )
        {
            if ( this->row( i ).get<2>().find( i ) == this->row( i ).get<2>().end() )
            {
                this->row( i ).get<2>().insert( i );
            }
        }

        else
        {
            this->row( i ).get<0>() = this->worldComm().globalRank(); //0;//rank
            this->row( i ).get<1>() = i-this->firstRowEntryOnProc(); //loc
            this->row( i ).get<2>().insert( i );
        }
    }
}


void
GraphCSR::close()
{
    if ( M_is_closed )
    {
        //std::cout << "already closed graph " << this << "...\n";
        return ;
    }

    M_is_closed = true;

    //std::cout << "closing graph " << this << "...\n";
    boost::timer ti;
    DVLOG(2) << "[close] nrows=" << this->size() << "\n";
    DVLOG(2) << "[close] firstRowEntryOnProc()=" << this->firstRowEntryOnProc() << "\n";
    DVLOG(2) << "[close] lastRowEntryOnProc()=" << this->lastRowEntryOnProc() << "\n";
    DVLOG(2) << "[close] firstColEntryOnProc()=" << this->firstColEntryOnProc() << "\n";
    DVLOG(2) << "[close] lastColEntryOnProc()=" << this->lastColEntryOnProc() << "\n";
    DVLOG(2) << "[close] M_n_total_nz=" << M_n_total_nz.size() << "\n";
    DVLOG(2) << "[close] M_storage size=" << M_storage.size() << "\n";
    DVLOG(2) << "[close] nrows=" << this->size() << "\n";
#if !defined(FEELPP_ENABLE_MPI_MODE)
    M_n_total_nz.resize( M_last_row_entry_on_proc+1/*M_storage.size()*/ );
    M_n_nz.resize( M_last_row_entry_on_proc+1/*M_storage.size()*/ );
    M_n_oz.resize( M_last_row_entry_on_proc+1/*M_storage.size()*/ );
#else // MPI
    M_n_total_nz.resize( this->lastRowEntryOnProc()-this->firstRowEntryOnProc()+1 );
    M_n_nz.resize( this->lastRowEntryOnProc()-this->firstRowEntryOnProc()+1 );
    M_n_oz.resize( this->lastRowEntryOnProc()-this->firstRowEntryOnProc()+1 );

    const int proc_id = this->worldComm().globalRank();
    const int nProc = this->worldComm().globalSize();

    //std::vector<int> nbMsgToSend( this->worldComm().globalSize() );
    //std::fill( nbMsgToSend.begin(),nbMsgToSend.end(),0 );
    //std::vector< std::map<int,int> > mapMsg( this->worldComm().globalSize() );
    std::vector<size_type> vecDofCol;//(1,0);

    std::vector< std::vector<size_type> > vecToSend( nProc );
    std::vector< std::vector<size_type> > vecToRecv( nProc );
    std::vector< std::vector<size_type> > vecToSend_nElt( nProc );
    std::vector< std::vector<size_type> > vecToRecv_nElt( nProc );

    std::vector< std::list<boost::tuple<size_type,std::vector<size_type> > > > memory_graphMPI( nProc );
    std::vector<size_type> memory_n_send(this->worldComm().globalSize() );

    for ( int proc=0 ; proc<nProc ; ++proc )
        {
            vecToSend[proc].clear();
            vecToRecv[proc].clear();
            vecToSend_nElt[proc].clear();
            vecToRecv_nElt[proc].clear();
        }
#endif

    std::fill( M_n_nz.begin(), M_n_nz.end(), 0 );
    std::fill( M_n_oz.begin(), M_n_oz.end(), 0 );

    size_type sum_nz = 0;

    M_max_nnz = 0;

    for ( auto it = M_storage.begin(), en = M_storage.end() ; it != en; ++it )
    {
        // Get the row of the sparsity pattern
        row_type const& irow = it->second;

        if ( ( int )boost::get<0>( irow ) == this->worldComm().globalRank() )
        {
            size_type globalindex = it->first;
            size_type localindex = irow.get<1>();
            size_type vec_size = irow.get<2>().size();

            FEELPP_ASSERT( globalindex >= firstRowEntryOnProc() )
            ( globalindex <= lastRowEntryOnProc() )
            ( globalindex )( firstRowEntryOnProc() )
            ( lastRowEntryOnProc() ).error ( "invalid local/global index" );
            FEELPP_ASSERT( globalindex >= 0 )( globalindex < M_n_total_nz.size() )
            ( globalindex )
            ( M_n_total_nz.size() ).error ( "invalid local/global index for M_n_total_nz" );
            M_n_total_nz[localindex] = vec_size;
            sum_nz += vec_size;

            for ( auto vecit = boost::get<2>( irow ).begin(), vecen = boost::get<2>( irow ).end(); vecit != vecen; ++vecit )
            {
#if defined(FEELPP_ENABLE_MPI_MODE) // MPI

                if ( ( *vecit < firstColEntryOnProc() ) ||
                     ( *vecit > lastColEntryOnProc() ) )
                {
                    // entry is off block-diagonal
                    ++M_n_oz[localindex];
                }

                else
#endif
                {
                    // entry is in block-diagonal
                    ++M_n_nz[localindex];
                }
            }


#if !defined( NDEBUG )
            DVLOG(2) << "M_total_nz [  " << localindex << "]=" << M_n_total_nz[localindex] << "\n";

            DVLOG(2) << "M_nz [  " << localindex << "]=" << M_n_nz[localindex] << "\n";
            DVLOG(2) << "M_oz [  " << localindex << "]=" << M_n_oz[localindex] << "\n";
#endif // NDEBUG

            M_max_nnz = std::max( M_n_total_nz[localindex], M_max_nnz );
        }

        else
        {
#if defined(FEELPP_ENABLE_MPI_MODE) // MPI
            // Get the row of the sparsity pattern
            const auto dofOnGlobalCluster = it->first;
            row_type const& irow = it->second;
            const auto procOnGlobalCluster = irow.get<0>();
            const auto dofOnProc = irow.get<1>();

            vecDofCol.resize( irow.get<2>().size()+2 );
            vecDofCol[0]=dofOnGlobalCluster;
            vecDofCol[1]=dofOnProc;
            auto icol_it = irow.get<2>().begin();
            auto icol_en = irow.get<2>().end();
            for ( int i=0; icol_it!=icol_en ; ++i,++icol_it )
            {
                vecDofCol[i+2] = *icol_it;
            }

#if 0
            std::cout << "/n I am proc " << this->worldComm().globalRank()
                      << " god Rank " << this->worldComm().godRank()
                      << " global Rank " << this->worldComm().globalRank()
                      << " I send to " << procOnGlobalCluster
                      << " with dofGlobCluster " << dofOnGlobalCluster
                      << " with dofLoc " << dofOnProc
                      << " size of ivec " << ivec.size()
                      << std::endl;
#endif

            memory_graphMPI[procOnGlobalCluster].push_back(boost::make_tuple( irow.get<1>(),vecDofCol));
            memory_n_send[procOnGlobalCluster]+=vecDofCol.size();

#endif // MPI
        }

    }


#if defined(FEELPP_ENABLE_MPI_MODE) // MPI

    if ( nProc > 1 )
        {
            // init container to send
            for ( int proc=0; proc<nProc; ++proc )
                {
                    vecToSend_nElt[proc].resize( memory_graphMPI[proc].size() );
                    vecToSend[proc].resize(memory_n_send[proc]);
                    auto vtsit = vecToSend[proc].begin();
                    auto it_mem = memory_graphMPI[proc].begin();
                    auto en_mem = memory_graphMPI[proc].end();
                    for ( int cpt = 0; it_mem !=en_mem ; ++it_mem)
                        {
                            vtsit = std::copy( boost::get<1>( *it_mem ).begin(), boost::get<1>( *it_mem ).end(), vtsit );
                            vecToSend_nElt[proc][cpt] = it_mem->get<1>().size();
                            ++cpt;
                        }
                }

            //------------------------------------------------------
            this->worldComm().globalComm().barrier();
            //------------------------------------------------------

            std::vector<size_type> nDataSize_vec(nProc);
            for ( int proc=0; proc<nProc; ++proc )
                {
                    const auto nDataSize = vecToSend[proc].size();
                    mpi::all_gather( this->worldComm().globalComm(),
                                     nDataSize,
                                     nDataSize_vec );

                    for ( int proc2=0; proc2<nProc; ++proc2 )
                        {
                            if ( nDataSize_vec[proc2] > 0 && proc!=proc2)
                                {
                                    if (proc_id==proc2) // send
                                        {
                                            this->worldComm().globalComm().send( proc, 0, vecToSend_nElt[proc] );
                                            this->worldComm().globalComm().send( proc, 1, vecToSend[proc] );
                                        }
                                    else if (proc_id==proc) // recv
                                        {
                                            this->worldComm().globalComm().recv( proc2, 0, vecToRecv_nElt[proc2] );
                                            this->worldComm().globalComm().recv( proc2, 1, vecToRecv[proc2] );
                                        }
                                }
                        }
                }

            //------------------------------------------------------
            this->worldComm().globalComm().barrier();
            //------------------------------------------------------
            for ( int proc=0; proc<nProc; ++proc )
                {
                    if (vecToRecv[proc].size()>0)
                        {
                            for (int cpt=0,istart=0;cpt<vecToRecv_nElt[proc].size();++cpt)
                                {
                                    const auto nRecvElt = vecToRecv_nElt[proc][cpt];

                                    size_type globalindex = vecToRecv[proc][istart];
                                    size_type localindex = vecToRecv[proc][istart+1];
                                    row_type& row = this->row( globalindex );
                                    row.get<0>()=proc_id;//this->worldComm().globalRank();
                                    row.get<1>()=localindex;

                                    for ( int k=2;k<nRecvElt;++k)
                                        {
                                            bool isInserted = false;
                                            if ( row.get<2>().find( vecToRecv[proc][istart+k] ) == row.get<2>().end() )
                                                {
                                                    row.get<2>().insert( vecToRecv[proc][istart+k] );
                                                    isInserted = true;
                                                }

                                            if ( ( vecToRecv[proc][istart+k] < firstColEntryOnProc() ) ||
                                                 ( vecToRecv[proc][istart+k] > lastColEntryOnProc() ) )
                                                {
                                                    if ( isInserted )
                                                        {
                                                            ++M_n_oz[localindex];
                                                            ++sum_nz;
                                                        }
                                                }

                                            else
                                                {
                                                    if ( isInserted )
                                                        {
                                                            ++M_n_nz[localindex];
                                                            ++sum_nz;
                                                        }
                                                }
                                        }

                                    istart += nRecvElt;
                                }
                        }
                }
        } //if ( nProc > 1 )
#endif // MPI_MODE



#if 1 // build ia,ja
#if 0
    M_ia.resize( M_storage.size()+1 );
    M_ja.resize( sum_nz );
    M_a.resize(  sum_nz, 0. );
    size_type col_cursor = 0;
    auto jait = M_ja.begin();

    for ( auto it = M_storage.begin(), en = M_storage.end()  ; it != en; ++it )
    {
        row_type const& irow = it->second;
        size_type localindex = boost::get<1>( irow );
        M_ia[localindex] = col_cursor;
        jait = std::copy( boost::get<2>( irow ).begin(), boost::get<2>( irow ).end(), jait );
        col_cursor+=boost::get<2>( irow ).size();
    }

    M_ia[M_storage.size()] = sum_nz;
#else

#if !defined(FEELPP_ENABLE_MPI_MODE) // NOT MPI
    M_ia.resize( M_last_row_entry_on_proc+2,0 );
    M_ja.resize( sum_nz );
    M_a.resize(  sum_nz, 0. );
    size_type col_cursor = 0;
    auto jait = M_ja.begin();
    //for( auto it = M_storage.begin(), en = M_storage.end()  ; it != en; ++it )

    for ( int i = 0 ; i<( M_last_row_entry_on_proc+1 ); ++i )
    {
        if ( M_storage.find( i )!=M_storage.end() )
        {
            row_type const& irow = this->row( i );
            size_type localindex = boost::get<1>( irow );
            M_ia[localindex] = col_cursor;
            jait = std::copy( boost::get<2>( irow ).begin(), boost::get<2>( irow ).end(), jait );
            col_cursor+=boost::get<2>( irow ).size();
        }

        else
        {
            M_ia[i] = col_cursor;
        }
    }

    M_ia[M_last_row_entry_on_proc+1] = sum_nz;

#else // MPI
    size_type nRowLoc = this->lastRowEntryOnProc()-this->firstRowEntryOnProc()+1;
    if ( nRowLoc>1 || ( sum_nz>0 )/* nRowLoc==1 && this->worldComm().globalRank()==4)*/ )
        {
            M_ia.resize( nRowLoc+1,0 );
            M_ja.resize( /*sum_n_nz*/sum_nz );
            M_a.resize(  /*sum_n_nz*/sum_nz, 0. );
            size_type col_cursor = 0;
            auto jait = M_ja.begin();
            for ( int i = 0 ; i< ( int ) nRowLoc; ++i )
                {
                    if ( M_storage.find( this->firstRowEntryOnProc()+i )!=M_storage.end() )
                        {

                            row_type const& irow = this->row( this->firstRowEntryOnProc()+i );
                            //size_type localindex = boost::get<1>( irow );
                            M_ia[i/*localindex*/] = col_cursor;
                            jait = std::copy( boost::get<2>( irow ).begin(), boost::get<2>( irow ).end(), jait );

                            col_cursor+=boost::get<2>( irow ).size();
                        }

                    else
                        {
                            //std::cout << "\n STRANGE " << std::endl;
                            M_ia[i] = col_cursor;
                        }
                }

            M_ia[nRowLoc] = /*sum_n_nz*/sum_nz;
        }
    else
        {
            M_ia.resize( 1,0 );
            M_ja.resize( 0 );
            M_a.resize(  0 );

        }
#endif // MPI
#endif // 0
#endif // 1 build ia,ja


} // close


void
GraphCSR::showMe( std::ostream& __out ) const
{
    __out << std::endl;
    this->worldComm().globalComm().barrier();

    for ( int proc = 0; proc<this->worldComm().globalSize(); ++proc )
    {
        if ( proc==this->worldComm().globalRank() )
        {
            __out << "--------------------------------------------------------------" << std::endl;
            __out << "-------------Graph (on proc " << proc << ")------------------------------"<< std::endl;
            __out << "first_row_entry_on_proc " << M_first_row_entry_on_proc << std::endl;
            __out << "last_row_entry_on_proc " << M_last_row_entry_on_proc << std::endl;
            __out << "first_col_entry_on_proc " << M_first_col_entry_on_proc << std::endl;
            __out << "last_col_entry_on_proc " << M_last_col_entry_on_proc << std::endl;
            __out << "max_nnz " << M_max_nnz << std::endl;

            for ( auto it = M_storage.begin(), en = M_storage.end() ; it != en; ++it )
            {
                // Get the row of the sparsity pattern
                row_type const& row = it->second;

                __out << " proc " << row.get<0>()
                      << " globalindex " << it->first
                      << " localindex " << row.get<1>();

                if ( it->first>=M_first_row_entry_on_proc && it->first<=M_last_row_entry_on_proc )
                {
                    __out << "(nz " << M_n_nz[row.get<1>()]
                          << " oz " << M_n_oz[row.get<1>()]
                          << ") : ";

                    //size_type vec_size = boost::get<2>( irow ).size();
                    //size_type globalindex = it->first;
                    //size_type localindex = boost::get<1>( irow );
                    for ( auto it = row.get<2>().begin(), en= row.get<2>().end() ; it!=en ; ++it )
                        __out << *it << " ";
                }

                __out << std::endl;
            }

            __out << "--------------------------------------------------------------" << std::endl;

        } // if (proc==this->comm().rank())

        this->worldComm().globalComm().barrier();

    } // for (int proc = 0;proc<nProc;++proc)

}

void
GraphCSR::printPython( std::string const& nameFile ) const
{

#if 0
    std::cout << "first_row_entry_on_proc " << M_first_row_entry_on_proc << std::endl;
    std::cout << "last_row_entry_on_proc " << M_last_row_entry_on_proc << std::endl;
    std::cout << "first_col_entry_on_proc " << M_first_col_entry_on_proc << std::endl;
    std::cout << "last_col_entry_on_proc " << M_last_col_entry_on_proc << std::endl;
    std::cout << "max_nnz " << M_max_nnz << std::endl;
#endif

    //compute first_row_entry last_row_entry on global graph
    std::vector<size_type> first_row_entry( this->worldComm().globalSize() );
    std::vector<size_type> last_row_entry( this->worldComm().globalSize() );
    std::vector<size_type> first_col_entry( this->worldComm().globalSize() );
    std::vector<size_type> last_col_entry( this->worldComm().globalSize() );
    mpi::all_gather( this->worldComm().globalComm(),
                     this->firstRowEntryOnProc(),
                     first_row_entry );
    mpi::all_gather( this->worldComm().globalComm(),
                     this->lastRowEntryOnProc(),
                     last_row_entry );
    mpi::all_gather( this->worldComm().globalComm(),
                     this->firstColEntryOnProc(),
                     first_col_entry );
    mpi::all_gather( this->worldComm().globalComm(),
                     this->lastColEntryOnProc(),
                     last_col_entry );
    size_type thefirstRow = *std::min_element( first_row_entry.begin(),first_row_entry.end() );
    size_type thelastRow = *std::max_element( last_row_entry.begin(),last_row_entry.end() );
    size_type thefirstCol = *std::min_element( first_col_entry.begin(),first_col_entry.end() );
    size_type thelastCol = *std::max_element( last_col_entry.begin(),last_col_entry.end() );


    //std::ofstream graphFile(nameFile, std::ios::out /*| std::ios::app*/);
    std::ofstream graphFile;//(nameFile, std::ios::out

    // start file : init
    if ( this->worldComm().globalRank() == this->worldComm().masterRank() )
    {
        graphFile.open( nameFile.c_str(), std::ios::out );

        graphFile << "import numpy" << std::endl
                  << "from scipy.sparse import * " << std::endl
                  << "from scipy import * " << std::endl
                  << "from pylab import * " << std::endl;

        graphFile << "nRow=" << thelastRow-thefirstRow+1 << std::endl
                  << "nCol=" << thelastCol-thefirstCol+1 << std::endl;

        graphFile << "mattt = array([" << std::endl;

        graphFile.close();
    }

    // synchro
    this->worldComm().barrier();

    // big part : graph
    for ( int proc = 0; proc<this->worldComm().globalSize(); ++proc )
    {
        if ( proc==this->worldComm().globalRank() )
        {
            graphFile.open( nameFile.c_str(), std::ios::out | std::ios::app );

            if (M_storage.size() > 0)
                {
                    for ( auto it = M_storage.begin(), en = --M_storage.end() ; it != en; ++it )
                        {
                            auto const& row = it->second;

                            if ( ( int )row.get<0>()==proc )
                                for ( auto it2 = row.get<2>().begin(), en2= row.get<2>().end() ; it2!=en2 ; ++it2 )
                                    graphFile << "[" << it->first << " , " << *it2 << " , 1.0 ],";// << std::endl;
                        }
                    auto it = --M_storage.end();
                    auto const& row = it->second;

                    if ( ( int )row.get<0>()==proc )
                        {
                            if ( row.get<2>().size()>0 )
                                {
                                    if ( row.get<2>().size()>1 )
                                        {
                                            for ( auto it2 = row.get<2>().begin(), en2= --row.get<2>().end() ; it2!=en2 ; ++it2 )
                                                graphFile << "[" << it->first << " , " << *it2 << " , 1.0 ],";
                                        }

                                    auto it2 = --row.get<2>().end();

                                    if ( proc==this->worldComm().globalSize()-1 || M_storage.size()==1 )
                                        graphFile << "[" << it->first << " , " << *it2 << " , 1.0 ] ])" << std::endl;

                                    else
                                        graphFile << "[" << it->first << " , " << *it2 << " , 1.0 ], " << std::endl;
                                }

                            else
                                {
                                    /*???*/
                                }
                        }
                }

            graphFile.close();
        }

        this->worldComm().barrier();
    }

    //endfile
    if ( this->worldComm().globalRank() == this->worldComm().masterRank() )
    {
        graphFile.open( nameFile.c_str(), std::ios::out | std::ios::app );

        graphFile << "row = array(mattt[:,0],dtype=int);" << std::endl
                  << "col = array(mattt[:,1],dtype=int);" << std::endl
                  << "data = array(mattt[:,2]);" << std::endl
                  << "A = csr_matrix( (data,(row,col)), shape=(nRow,nCol) );" << std::endl
                  << "fig = plt.figure();" << std::endl
                  << "matplotlib.pyplot.spy(A,precision=1e-8,aspect='equal');" << std::endl
                  << "plt.show();" << std::endl;

        graphFile.close();

    }


} // printPython


} // Feel
