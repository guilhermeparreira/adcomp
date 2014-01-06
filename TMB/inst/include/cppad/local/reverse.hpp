/* $Id: reverse.hpp 2625 2012-12-23 14:34:12Z bradbell $ */
# ifndef CPPAD_REVERSE_INCLUDED
# define CPPAD_REVERSE_INCLUDED

/* --------------------------------------------------------------------------
CppAD: C++ Algorithmic Differentiation: Copyright (C) 2003-12 Bradley M. Bell

CppAD is distributed under multiple licenses. This distribution is under
the terms of the 
                    GNU General Public License Version 3.

A copy of this license is included in the COPYING file of this distribution.
Please visit http://www.coin-or.org/CppAD/ for information on other licenses.
-------------------------------------------------------------------------- */

/*
$begin Reverse$$
$spell
	typename
	xk
	xp
	dw
	Ind
	uj
	std
	arg
	const
	taylor_
$$

$section Reverse Mode$$ 

$childtable%
	omh/reverse.omh
%$$

$end
-----------------------------------------------------------------------------
*/
# include <algorithm>
# include <cppad/local/pod_vector.hpp>

CPPAD_BEGIN_NAMESPACE
/*!
\defgroup reverse_hpp reverse.hpp
\{
\file reverse.hpp
Compute derivatives using reverse mode.
*/


/*!
Use reverse mode to compute derivative of forward mode Taylor coefficients.

The function 
\f$ X : {\rm R} \times {\rm R}^{n \times p} \rightarrow {\rm R} \f$ 
is defined by
\f[
X(t , u) = \sum_{k=0}^{p-1} u^{(k)} t^k
\f]
The function 
\f$ Y : {\rm R} \times {\rm R}^{n \times p} \rightarrow {\rm R} \f$ 
is defined by
\f[
Y(t , u) = F[ X(t, u) ]
\f]
The function 
\f$ W : {\rm R}^{n \times p} \rightarrow {\rm R} \f$ is defined by
\f[
W(u) = \sum_{k=0}^{p-1} ( w^{(k)} )^{\rm T} 
\frac{1}{k !} \frac{ \partial^k } { t^k } Y(0, u)
\f]

\tparam Base
base type for the operator; i.e., this operation sequence was recorded
using AD< \a Base > and computations by this routine are done using type 
\a Base.

\tparam VectorBase
is a Simple Vector class with elements of type \a Base.

\param p
is the number of the number of Taylor coefficients that are being
differentiated (per variable).

\param w
is the weighting for each of the Taylor coefficients corresponding
to dependent variables.
If the argument \a w has size <tt>m * p </tt>,
for \f$ k = 0 , \ldots , p-1 \f$ and \f$ i = 0, \ldots , m-1 \f$,
\f[
	w_i^{(k)} = w [ i * p + k ]
\f]
If the argument \a w has size \c m ,
for \f$ k = 0 , \ldots , p-1 \f$ and \f$ i = 0, \ldots , m-1 \f$,
\f[
w_i^{(k)} = \left\{ \begin{array}{ll}
	w [ i ] & {\rm if} \; k = p-1
	\\
	0       & {\rm otherwise}
\end{array} \right.
\f]

\return
Is a vector \f$ dw \f$ such that
for \f$ j = 0 , \ldots , n-1 \f$ and
\f$ k = 0 , \ldots , p-1 \f$
\f[ 
	dw[ j * p + k ] = W^{(1)} ( x )_{j,k}
\f]
where the matrix \f$ x \f$ is the value for \f$ u \f$
that corresponding to the forward mode Taylor coefficients
for the independent variables as specified by previous calls to Forward.

*/
template <typename Base>
template <typename VectorBase>
VectorBase ADFun<Base>::Reverse(size_t p, const VectorBase &w) 
{	// constants
	const Base zero(0);

	// temporary indices
	size_t i, j, k;

	// number of independent variables
	size_t n = ind_taddr_.size();

	// number of dependent variables
	size_t m = dep_taddr_.size();

	pod_vector<Base> Partial;
	Partial.extend(total_num_var_ * p);

	// update maximum memory requirement
	// memoryMax = std::max( memoryMax, 
	// 	Memory() + total_num_var_ * p * sizeof(Base)
	// );

	// check VectorBase is Simple Vector class with Base type elements
	CheckSimpleVector<Base, VectorBase>();

	CPPAD_ASSERT_KNOWN(
		size_t(w.size()) == m || size_t(w.size()) == (m * p),
		"Argument w to Reverse does not have length equal to\n"
		"the dimension of the range for the corresponding ADFun."
	);
	CPPAD_ASSERT_KNOWN(
		p > 0,
		"The first argument to Reverse must be greater than zero."
	);  
	CPPAD_ASSERT_KNOWN(
		taylor_per_var_ >= p,
		"Less that p taylor_ coefficients are currently stored"
		" in this ADFun object."
	);  

	// initialize entire Partial matrix to zero
	for(i = 0; i < total_num_var_; i++)
		for(j = 0; j < p; j++)
			Partial[i * p + j] = zero;

	// set the dependent variable direction
	// (use += because two dependent variables can point to same location)
	for(i = 0; i < m; i++)
	{	CPPAD_ASSERT_UNKNOWN( dep_taddr_[i] < total_num_var_ );
		if( size_t(w.size()) == m )
			Partial[dep_taddr_[i] * p + p - 1] += w[i];
		else
		{	for(k = 0; k < p; k++)
				// ? should use += here, first make test to demonstrate bug
				Partial[ dep_taddr_[i] * p + k ] = w[i * p + k ];
		}
	}

	// evaluate the derivatives
	ReverseSweep(
		p - 1,
		n,
		total_num_var_,
		&play_,
		taylor_col_dim_,
		taylor_.data(),
		p,
		Partial.data()
	);

	// return the derivative values
	VectorBase value(n * p);
	for(j = 0; j < n; j++)
	{	CPPAD_ASSERT_UNKNOWN( ind_taddr_[j] < total_num_var_ );

		// independent variable taddr equals its operator taddr 
		CPPAD_ASSERT_UNKNOWN( play_.GetOp( ind_taddr_[j] ) == InvOp );

		// by the Reverse Identity Theorem 
		// partial of y^{(k)} w.r.t. u^{(0)} is equal to
		// partial of y^{(p-1)} w.r.t. u^{(p - 1 - k)}
		if( size_t(w.size()) == m )
		{	for(k = 0; k < p; k++)
				value[j * p + k ] = 
					Partial[ind_taddr_[j] * p + p - 1 - k];
		}
		else
		{	for(k = 0; k < p; k++)
				value[j * p + k ] =
					Partial[ind_taddr_[j] * p + k];
		}
	}

	return value;
}


/*  =========================== kaspers Reverse */
template <typename Base>
template <typename VectorBase>
//VectorBase ADFun<Base>::myReverse(size_t p, const VectorBase &w, size_t dep_var_index, VectorBase &value) 
void ADFun<Base>::myReverse(size_t p, const VectorBase &w, size_t dep_var_index, VectorBase &value) 
{
	CPPAD_ASSERT_KNOWN(
		p == 1,
		"myReverse only works for first order calculations."
	);  

	// constants
	const Base zero(0);

	// temporary indices
	size_t j, k;

	// number of independent variables
	size_t n = ind_taddr_.size();

	// number of dependent variables
	// size_t m = dep_taddr_.size();

	if(false){
	  pod_vector<Base> Partial;
	  Partial.extend(total_num_var_ * p);
	  }

	// update maximum memory requirement
	// memoryMax = std::max( memoryMax, 
	// 	Memory() + total_num_var_ * p * sizeof(Base)
	// );

	// check VectorBase is Simple Vector class with Base type elements
	CheckSimpleVector<Base, VectorBase>();

	CPPAD_ASSERT_KNOWN(
		w.size() == m || w.size() == (m * p),
		"Argument w to Reverse does not have length equal to\n"
		"the dimension of the range for the corresponding ADFun."
	);
	CPPAD_ASSERT_KNOWN(
		p > 0,
		"The first argument to Reverse must be greater than zero."
	);  
	CPPAD_ASSERT_KNOWN(
		taylor_per_var_ >= p,
		"Less that p taylor_ coefficients are currently stored"
		" in this ADFun object."
	);  

	/*
	// initialize entire Partial matrix to zero
	for(i = 0; i < total_num_var_; i++)
		for(j = 0; j < p; j++)
			Partial[i * p + j] = zero;

	// set the dependent variable direction
	// (use += because two dependent variables can point to same location)
	for(i = 0; i < m; i++)
	{	CPPAD_ASSERT_UNKNOWN( dep_taddr_[i] < total_num_var_ );
		if( w.size() == m )
			Partial[dep_taddr_[i] * p + p - 1] += w[i];
		else
		{	for(k = 0; k < p; k++)
				// ? should use += here, first make test to demonstrate bug
				Partial[ dep_taddr_[i] * p + k ] = w[i * p + k ];
		}
	}
	*/
	size_t dep_var_taddr=dep_taddr_[dep_var_index];
	Partial[dep_var_taddr * p + p - 1] = 1.0;

	// evaluate the derivatives
	// vector<size_t> relevant(play_.num_rec_var());
	myReverseSweep(
		p - 1,
		n,
		total_num_var_,
		&play_,
		taylor_col_dim_,
		taylor_.data(),
		p,
		Partial.data(),
		//dep_var_taddr,
		dep_var_index,
		// op_mark_,
		// var2op_,
		// tp_,
		this
	);


	// return the derivative values
	// VectorBase value(n * p);
    std::vector<size_t>::iterator it;
    for(it=op_inv_index_.begin();it!=op_inv_index_.end();it++){
      //colpattern[col][j]=*it-1;
      j=*it-1;
	    for(k = 0; k < p; k++)
	      value[j * p + k ] = 
		Partial[ind_taddr_[j] * p + p - 1 - k];
    }

    if(false){
	for(j = 0; j < n; j++){
	  //if(relevant_[ ind_taddr_[j] ]==dep_var_taddr)
	  if(op_mark_[ var2op_[ ind_taddr_[j] ] ]==dep_var_taddr)
	    {

	    for(k = 0; k < p; k++)
	      value[j * p + k ] = 
		Partial[ind_taddr_[j] * p + p - 1 - k];
	  
	  }
	}
    }

    // Fill used Partials with zeros
    tape_point tp;
    for(it=op_mark_index_.begin();it!=op_mark_index_.end();it++){
      tp=tp_[*it];
      for(size_t i=0;i<CppAD::NumRes(tp.op);i++)
	for(j = 0; j < p; j++)
	  Partial[tp.var_index-i*p+j]=0;
    }

#ifdef DEBUG_KASPER
    int countnnz=0;
    for(i = 0; i < total_num_var_; i++)
      for(j = 0; j < p; j++)
	countnnz+=(Partial[i * p + j] != zero);
    if(countnnz>0){
      std::cout << "Partials not correctly cleared. Nonzeros: " << countnnz << "\n";
    }
#endif

	// EXPERIMENT
	/*
	for(j = 0; j < n; j++)
	{	CPPAD_ASSERT_UNKNOWN( ind_taddr_[j] < total_num_var_ );

		// independent variable taddr equals its operator taddr 
		CPPAD_ASSERT_UNKNOWN( play_.GetOp( ind_taddr_[j] ) == InvOp );

		// by the Reverse Identity Theorem 
		// partial of y^{(k)} w.r.t. u^{(0)} is equal to
		// partial of y^{(p-1)} w.r.t. u^{(p - 1 - k)}
		if( w.size() == m )
		{	for(k = 0; k < p; k++)
				value[j * p + k ] = 
					Partial[ind_taddr_[j] * p + p - 1 - k];
		}
		else
		{	for(k = 0; k < p; k++)
				value[j * p + k ] =
					Partial[ind_taddr_[j] * p + k];
		}
	}
	*/

	//return value;
}
/* =================== End kaspers Reverse */
	

/*! \} */
CPPAD_END_NAMESPACE
# endif
