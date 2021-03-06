/*numeric lu factorization, kernel functions*/
/*last modified: june 16, 2013*/
/*author: Chen, Xiaoming*/

#include "graphlu.h"
#include "graphlu_internal.h"
#include "timer_c.h"
#include "graphlu_default.h"
#ifdef SSE2
#include <emmintrin.h>
#endif

/*lu data structure*/
/*row 0: all u indexes    all u data     all l indexes    all l data*/
/*row 1: all u indexes    all u data     all l indexes    all l data*/
/*......*/

extern size_t g_sd, g_si, g_sp, g_sis1, g_sps1;

int GraphLU_Factorize(SGraphLU *graphlu)
{
	real__t *ax;
	uint__t *ai, *ap;
	uint__t cstart, cend, oldrow;
	uint__t *rowperm;
	size_t lnnz, unnz;/*the actual nnz*/
	real__t *ldiag;
	void *lu_array;
	size_t *up;
	uint__t *llen, *ulen;
	uint__t n;
	int__t *p, *pinv;
	real__t tol;
	uint__t offdp;
	uint__t scale;
	real__t *cscale;
	real__t *x;
	int__t *flag, *pend, *stack, *appos;
	size_t size;
	size_t m0, m1, used;
	uint__t i, j, jnew, k, q, kb;
	real__t xj;
	int__t top;
	int__t diagcol;
	uint__t pivcol;
	real__t pivot;
	uint__t ul, ll;
	uint__t *row_index;
	real__t *row_data;
	uint__t *u_row_index_j;
	real__t *u_row_data_j;
	uint__t u_row_len_j;
	size_t est;
	int err;
	real__t grow;
#ifdef SSE2
	__m128d _xj, _prod, _tval;
	uint__t _end;
#endif


	/*check flags*/
	if (NULL == graphlu)
	{
		return GRAPHLU_ARGUMENT_ERROR;
	}
	graphlu->flag[2] = FALSE;
	if (!graphlu->flag[1])
	{
		return GRAPHLU_MATRIX_NOT_ANALYZED;
	}
	/*set parameters*/
	n = graphlu->n;
	ax = graphlu->ax;
	ai = graphlu->ai;
	ap = graphlu->ap;
	rowperm = graphlu->row_perm;
	lnnz = 0;
	unnz = 0;
	ldiag = graphlu->ldiag;
	up = graphlu->up;
	llen = graphlu->llen;
	ulen = graphlu->ulen;
	p = graphlu->pivot;
	pinv = graphlu->pivot_inv;
	tol = graphlu->cfgf[0];
/*
	for (i = 0; i < graphlu->nnz; ++i)
	{ 
		printf("Debug: ai[%d]: %d\n", i, ai[i]);
	}
	for (i = 0; i < n; ++i)
	{
		printf("Debug: ldiag[%d]: %d", i, ldiag[i]);
		printf("Debug: up[%d]: %d", i, up[i]);
		printf("Debug: llen[%d]: %d", i, llen[i]);
		printf("Debug: ulen[%d]: %d", i, ulen[i]);
		printf("Debug: pivot[%d]: %d \n", i, p[i]);
	}
*/
	if (tol <= 1.e-32)
	{
		tol = 1.e-32;
		graphlu->cfgf[0] = tol;
	}
	else if (tol > 0.99999999)
	{
		tol = 0.99999999;
		graphlu->cfgf[0] = tol;
	}
	offdp = 0;
	scale = graphlu->cfgi[2];
	cscale = graphlu->cscale;
	grow = graphlu->cfgf[5];
/*  
        printf("Debug: scale: %d, grow: %f\n", scale, grow);
	for (i = 0; i < n; ++i)
        {
                printf("Debug: cscale[%d]: %f\n ", i, cscale[i]);
        }
*/
	if (grow <= 1.)
	{
		grow = GRAPHLU_MEMORY_GROW;
		graphlu->cfgf[5] = grow;
	}

	/*begin*/
	TimerStart((STimer *)(graphlu->timer));

	/*mark all columns as non-pivotal*/
	for (i=0; i<n; ++i)
	{
		p[i] = i;
		pinv[i] = -((int__t)i)-2;
	}

	/*work*/
	/*|-----|-----|-----|-----|-----| */
	/* x     flag  pend  stack appos */
	x = (real__t *)(graphlu->workspace);
	flag = (int__t *)(x + n);
	pend = flag + n;
	stack = pend + n;
	appos = stack + n;

	memset(x, 0, sizeof(real__t)*n);
	memset(flag, 0xff, sizeof(int__t)*(n+n));/*clear flag and pend*/

//	printf("Debug: factorization lu_nnz_est: %zu", graphlu->lu_nnz_est);
//	printf("Debug: g_sp: %zu", g_sp);
	/*alloc lu data*/
	if (graphlu->lu_array == NULL)
	{
		est = graphlu->lu_nnz_est;
		size = g_sp*est;
		lu_array = malloc(size);
		if (NULL == lu_array)
		{
			return GRAPHLU_MEMORY_OVERFLOW;
		}
		graphlu->lu_array = lu_array;
		m0 = size;
		used = 0;
	}
	else
	{
		est = graphlu->lu_nnz_est;
		size = g_sp*est;
		lu_array = realloc(graphlu->lu_array, size);
		if (NULL == lu_array)
		{
			return GRAPHLU_MEMORY_OVERFLOW;
		}
		graphlu->lu_array = lu_array;
		m0 = size;
		used = 0;
	}

	/*numeric factorize*/
	for (i=0; i<n; ++i)
	{
		up[i] = used;
		oldrow = rowperm[i];
		cstart = ap[oldrow];
		cend = ap[oldrow+1];

//		printf("Debug: rowperm[%d]: %d,	ap[%d]: %d\n", i, oldrow, oldrow, cstart);
		/*estimate the length*/
		m1 = used + n*g_sp;

		if (m1 > m0)
		{

			m0 = ( ( ((size_t)(m0 * grow)) + g_sps1) / g_sp) * g_sp;
			
			if (m1 > m0)
			{
				m0 = ( ( ((size_t)(m1 * grow)) + g_sps1) / g_sp) * g_sp;
			}

			lu_array = realloc(lu_array, m0);
			if (NULL == lu_array)
			{
				return GRAPHLU_MEMORY_OVERFLOW;
			}
			graphlu->lu_array = lu_array;
			est = m0/g_sp;
			graphlu->lu_nnz_est = est;
		}

		/*symbolic*/
		top = _I_GraphLU_Symbolic(n, i, pinv, stack, flag, pend, appos, \
			(uint__t *)(((byte__t *)lu_array) + up[i]), ulen, lu_array, up, &ai[cstart], cend-cstart);
//		printf("Debug: top: %d", top);
		/*numeric*/
		if (scale == 1 || scale == 2)
		{
			for (k=cstart; k<cend; ++k)
			{
				j = ai[k];
				x[j] = ax[k] / cscale[j];
			}
		}
		else
		{
			for (k=cstart; k<cend; ++k)
			{
				x[ai[k]] = ax[k];
			}
		}

		for (k=top; k<n; ++k)
		{
			j = stack[k];
			jnew = pinv[j];
			
			/*extract row jnew of U*/
			u_row_len_j = ulen[jnew];
			u_row_index_j = (uint__t *)(((byte__t *)lu_array) + up[jnew]);
			u_row_data_j = (real__t *)(u_row_index_j + u_row_len_j);

			xj = x[j];
#ifndef SSE2
			for (q=0; q<u_row_len_j; ++q)
			{
				x[u_row_index_j[q]] -= xj * u_row_data_j[q];
			}
#else
			_xj = _mm_load1_pd(&xj);
			_end = (u_row_len_j&(uint__t)1)>0 ? u_row_len_j-1 : u_row_len_j;
			for (q=0; q<_end; q+=2)
			{
				_tval = _mm_loadu_pd(&(u_row_data_j[q]));
				_prod = _mm_mul_pd(_xj, _tval);
				_tval = _mm_load_sd(&(x[u_row_index_j[q]]));
				_tval = _mm_loadh_pd(_tval, &(x[u_row_index_j[q+1]]));
				_tval = _mm_sub_pd(_tval, _prod);
				_mm_storel_pd(&(x[u_row_index_j[q]]), _tval);
				_mm_storeh_pd(&(x[u_row_index_j[q+1]]), _tval);
			}
			if ((u_row_len_j&(uint__t)1) > 0)
			{
				x[u_row_index_j[_end]] -= xj * u_row_data_j[_end];
			}
#endif
		}

		/*pivoting*/
		diagcol = p[i];/*column diagcol is the ith pivot*/
		err = _I_GraphLU_Pivot(diagcol, &ulen[i], up[i], tol, x, \
			&pivcol, &pivot, lu_array);
//		printf("cfgi[9] = %d\n", graphlu->cfgi[9]);
		if (graphlu->cfgi[9] == 1) {
			if (err == GRAPHLU_MATRIX_NUMERIC_SINGULAR) {
				pivot = 1.0;
				printf("set pivot to 1\n");
				graphlu->singular[i] = 1;
			}
		} else if (FAIL(err)) {
			return err;
		}

		/*update up, ux, lp, lx, ulen, llen*/
		ll = llen[i] = n-top;
		ul = ulen[i];

		row_index = (uint__t *)(((byte__t *)lu_array) + up[i] + ul*g_sp);
		row_data = (real__t *)(row_index + ll);

		/*push into L*/
		for (k=top, q=0; k<n; ++k, ++q)
		{
			j = stack[k];
			row_index[q] = pinv[j];/*!!! L is put in pivoting order here*/
			row_data[q] = x[j];
			x[j] = 0.;
		}

		ldiag[i] = pivot;

		lnnz += ll;
		unnz += ul;

		used += (ul+ll) * g_sp;

		/*log the pivoting*/
		if (pivcol != diagcol)/*diagcol = p[i]*/
		{
			++offdp;
			if (pinv[diagcol] < 0)
			{
				kb = -pinv[pivcol]-2;/*pinv[pivcol] must < 0*/
				p[kb] = diagcol;
				pinv[diagcol] = -(int__t)kb-2;
			}
		}
		p[i] = pivcol;
		pinv[pivcol] = i;
		
		/*prune*/
		_I_GraphLU_Prune(pend, ll, ulen, pinv, pivcol, row_index, up, lu_array);
	}

	/*put U in the pivoting order*/
	for (k=0; k<n; ++k)
	{
		row_index = (uint__t *)(((byte__t *)lu_array) + up[k]);
		ll = ulen[k];

		for (i=0; i<ll; ++i)
		{
			row_index[i] = pinv[row_index[i]];
		}
	}

	graphlu->l_nnz = lnnz + n;
	graphlu->u_nnz = unnz + n;
	graphlu->lu_nnz = lnnz + unnz + n;
	graphlu->stat[14] = (real__t)offdp;
	graphlu->stat[26] = (real__t)(graphlu->l_nnz);
	graphlu->stat[27] = (real__t)(graphlu->u_nnz);
	graphlu->stat[28] = (real__t)(graphlu->lu_nnz);

	graphlu->flag[2] = TRUE;

	/*finish*/
	TimerStop((STimer *)(graphlu->timer));
	graphlu->stat[1] = TimerGetRuntime((STimer *)(graphlu->timer));

	return GRAPH_OK;
}
