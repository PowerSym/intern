/*solve Ly=b and Ux=y*/
/*last modified: june 7, 2013*/
/*author: Chen, Xiaoming*/

#include "nicslu.h"
#include "nicslu_internal.h"
#include "timer_c.h"

extern size_t g_si, g_sd, g_sp;

int NicsLU_Solve(SNicsLU *nicslu, real__t *rhs)
{
	uint__t mc64_scale, scale;
	real__t *rows, *cols;
	uint__t j, p, n;
	int__t jj;
	real__t *b;
	uint__t *rp, *cp;
	real__t sum;
	size_t *up;
	uint__t *ip;
	real__t *x;
	uint__t ul, ll;
	uint__t *llen, *ulen;
	real__t *ldiag;
	void *lu;
	uint__t *piv;
	real__t *cscale;

	/*check inputs*/
	if (NULL == nicslu || NULL == rhs)
	{
		return NICSLU_ARGUMENT_ERROR;
	}
	if (!nicslu->flag[2])
	{
		return NICSLU_MATRIX_NOT_FACTORIZED;
	}

	/*begin*/
	TimerStart((STimer *)(nicslu->timer));

	n = nicslu->n;
	b = nicslu->rhs;
	up = nicslu->up;
	llen = nicslu->llen;
	ulen = nicslu->ulen;
	ldiag = nicslu->ldiag;
	lu = nicslu->lu_array;
	mc64_scale = nicslu->cfgi[1];
	scale = nicslu->cfgi[2];
	cscale = nicslu->cscale;

	if (nicslu->cfgi[0] == 0)
	{
		rp = nicslu->row_perm;
		cp = nicslu->col_perm_inv;
		piv = (uint__t *)(nicslu->pivot_inv);
		rows = nicslu->row_scale;
		cols = nicslu->col_scale_perm;

		if (mc64_scale)
		{
			for (j=0; j<n; ++j)
			{
				p = rp[j];
				b[j] = rhs[p] * rows[p];
			}
		}
		else
		{
			for (j=0; j<n; ++j)
			{
				b[j] = rhs[rp[j]];
			}
		}

		for (j=0; j<n; ++j)
		{
			sum = 0.;
			ll = llen[j];
			ul = ulen[j];

			ip = (uint__t *)(((byte__t *)lu) + up[j] + ul*g_sp);
			x = (real__t *)(ip + ll);

			for (p=0; p<ll; ++p)
			{
				sum += x[p] * b[ip[p]];
			}

			b[j] -= sum;
			b[j] /= ldiag[j];
		}

		for (jj=n-1; jj>=0; --jj)
		{
			sum = 0.;
			ul = ulen[jj];

			ip = (uint__t *)(((byte__t *)lu) + up[jj]);
			x = (real__t *)(ip + ul);

			for (p=0; p<ul; ++p)
			{
				sum += x[p] * b[ip[p]];
			}

			b[jj] -= sum;
		}

		if (scale == 1 || scale == 2)
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					rhs[j] = b[piv[p]] * cols[p] / cscale[p];
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					rhs[j] = b[piv[p]] / cscale[p];
				}
			}
		}
		else
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					rhs[j] = b[piv[p]] * cols[p];
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					rhs[j] = b[piv[cp[j]]];
				}
			}
		}
	}
	else/*CSC*/
	{
		rp = nicslu->col_perm;
		cp = nicslu->row_perm_inv;
		piv = (uint__t *)(nicslu->pivot);
		rows = nicslu->col_scale_perm;
		cols = nicslu->row_scale;

		if (scale == 1 || scale == 2)
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					b[j] = rhs[rp[p]] * rows[p] / cscale[p];
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					b[j] = rhs[rp[p]] / cscale[p];
				}
			}
		}
		else
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					b[j] = rhs[rp[p]] * rows[p];
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					b[j] = rhs[rp[piv[j]]];
				}
			}
		}

		for (j=0; j<n; ++j)
		{
			sum = b[j];
			ul = ulen[j];

			ip = (uint__t *)(((byte__t *)lu) + up[j]);
			x = (real__t *)(ip + ul);

			for (p=0; p<ul; ++p)
			{
				b[ip[p]] -= x[p] * sum;
			}
		}

		for (jj=n-1; jj>=0; --jj)
		{
			sum = b[jj];
			ll = llen[jj];
			ul = ulen[jj];

			ip = (uint__t *)(((byte__t *)lu) + up[jj] + ul*g_sp);
			x = (real__t *)(ip + ll);

			sum /= ldiag[jj];
			b[jj] = sum;

			for (p=0; p<ll; ++p)
			{
				b[ip[p]] -= x[p] * sum;
			}
		}

		if (mc64_scale)
		{
			for (j=0; j<n; ++j)
			{
				rhs[j] = b[cp[j]] * cols[j];
			}
		}
		else
		{
			for (j=0; j<n; ++j)
			{
				rhs[j] = b[cp[j]];
			}
		}
	}

	/*finish*/
	TimerStop((STimer *)(nicslu->timer));
	nicslu->stat[3] = TimerGetRuntime((STimer *)(nicslu->timer));

	return NICS_OK;
}

int NicsLU_SolveFast(SNicsLU *nicslu, real__t *rhs)
{
	uint__t mc64_scale, scale;
	real__t *rows, *cols;
	uint__t j, p, n;
	int__t jj;
	real__t *b;
	uint__t *rp, *cp;
	real__t sum;
	size_t *up;
	uint__t *ip;
	real__t *x;
	uint__t ul, ll;
	uint__t *llen, *ulen;
	real__t *ldiag;
	void *lu;
	uint__t *piv;
	real__t *cscale;
	int__t first;
	uint__t col;
	real__t val;

	/*check inputs*/
	if (NULL == nicslu || NULL == rhs)
	{
		return NICSLU_ARGUMENT_ERROR;
	}
	if (!nicslu->flag[2])
	{
		return NICSLU_MATRIX_NOT_FACTORIZED;
	}

	/*begin*/
	TimerStart((STimer *)(nicslu->timer));

	n = nicslu->n;
	b = nicslu->rhs;
	up = nicslu->up;
	llen = nicslu->llen;
	ulen = nicslu->ulen;
	ldiag = nicslu->ldiag;
	lu = nicslu->lu_array;
	mc64_scale = nicslu->cfgi[1];
	scale = nicslu->cfgi[2];
	cscale = nicslu->cscale;
	first = -1;

	if (nicslu->cfgi[0] == 0)
	{
		rp = nicslu->row_perm;
		cp = nicslu->col_perm_inv;
		piv = (uint__t *)(nicslu->pivot_inv);
		rows = nicslu->row_scale;
		cols = nicslu->col_scale_perm;

		if (mc64_scale)
		{
			for (j=0; j<n; ++j)
			{
				p = rp[j];
				b[j] = rhs[p] * rows[p];
				if (first < 0 && b[j] != 0.) first = j;
			}
		}
		else
		{
			for (j=0; j<n; ++j)
			{
				b[j] = rhs[rp[j]];
				if (first < 0 && b[j] != 0.) first = j;
			}
		}

		/*all 0*/
		if (first < 0)
		{
			memset(rhs, 0, sizeof(real__t)*n);
			return NICS_OK;
		}

		for (j=first; j<n; ++j)
		{
			sum = 0.;
			ll = llen[j];
			ul = ulen[j];

			ip = (uint__t *)(((byte__t *)lu) + up[j] + ul*g_sp);
			x = (real__t *)(ip + ll);

			for (p=0; p<ll; ++p)
			{
				col = ip[p];
				val = b[col];
				if (col >= (uint__t)first && val != 0.) sum += x[p] * val;
			}

			b[j] -= sum;
			b[j] /= ldiag[j];
		}

		for (jj=n-1; jj>=0; --jj)
		{
			sum = 0.;
			ul = ulen[jj];

			ip = (uint__t *)(((byte__t *)lu) + up[jj]);
			x = (real__t *)(ip + ul);

			for (p=0; p<ul; ++p)
			{
				val = b[ip[p]];
				if (val != 0.) sum += x[p] * val;
			}

			b[jj] -= sum;
		}

		if (scale == 1 || scale == 2)
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					rhs[j] = b[piv[p]] * cols[p] / cscale[p];
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					rhs[j] = b[piv[p]] / cscale[p];
				}
			}
		}
		else
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					rhs[j] = b[piv[p]] * cols[p];
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					rhs[j] = b[piv[cp[j]]];
				}
			}
		}
	}
	else/*CSC*/
	{
		rp = nicslu->col_perm;
		cp = nicslu->row_perm_inv;
		piv = (uint__t *)(nicslu->pivot);
		rows = nicslu->col_scale_perm;
		cols = nicslu->row_scale;

		if (scale == 1 || scale == 2)
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					b[j] = rhs[rp[p]] * rows[p] / cscale[p];
					if (first < 0 && b[j] != 0.) first = j;
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					b[j] = rhs[rp[p]] / cscale[p];
					if (first < 0 && b[j] != 0.) first = j;
				}
			}
		}
		else
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					b[j] = rhs[rp[p]] * rows[p];
					if (first < 0 && b[j] != 0.) first = j;
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					b[j] = rhs[rp[piv[j]]];
					if (first < 0 && b[j] != 0.) first = j;
				}
			}
		}

		/*all 0*/
		if (first < 0)
		{
			memset(rhs, 0, sizeof(real__t)*n);
			return NICS_OK;
		}

		for (j=(uint__t)first; j<n; ++j)
		{
			sum = b[j];

			if (sum != 0.)
			{
				ul = ulen[j];

				ip = (uint__t *)(((byte__t *)lu) + up[j]);
				x = (real__t *)(ip + ul);

				for (p=0; p<ul; ++p)
				{
					b[ip[p]] -= x[p] * sum;
				}
			}
		}

		for (jj=n-1; jj>=0; --jj)
		{
			sum = b[jj];

			if (sum != 0.)
			{
				sum /= ldiag[jj];
				b[jj] = sum;

				ll = llen[jj];
				ul = ulen[jj];

				ip = (uint__t *)(((byte__t *)lu) + up[jj] + ul*g_sp);
				x = (real__t *)(ip + ll);

				for (p=0; p<ll; ++p)
				{
					b[ip[p]] -= x[p] * sum;
				}
			}
			else
			{
				b[jj] = 0.;
			}
		}

		if (mc64_scale)
		{
			for (j=0; j<n; ++j)
			{
				rhs[j] = b[cp[j]] * cols[j];
			}
		}
		else
		{
			for (j=0; j<n; ++j)
			{
				rhs[j] = b[cp[j]];
			}
		}
	}

	/*finish*/
	TimerStop((STimer *)(nicslu->timer));
	nicslu->stat[3] = TimerGetRuntime((STimer *)(nicslu->timer));

	return NICS_OK;
}

// Solve singular linear system with given reference solution
int NicsLU_Solve_Singular(SNicsLU *nicslu, real__t *rhs, real__t *ref)
{
	uint__t mc64_scale, scale;
	real__t *rows, *cols;
	uint__t j, p, n;
	int__t jj;
	real__t *b;
	uint__t *rp, *cp;
	real__t sum;
	size_t *up;
	uint__t *ip;
	real__t *x;
	uint__t ul, ll;
	uint__t *llen, *ulen;
	real__t *ldiag;
	void *lu;
	uint__t *piv;
	real__t *cscale;
	int__t *singular;
	int__t isSingular = 0;

	//printf("NicsLU_Solve_Singular:::\n");

	/*check inputs*/
	if (NULL == nicslu || NULL == rhs)
	{
		//printf("NicsLU_Solve_Singular: NICSLU_ARGUMENT_ERROR\n");
		return NICSLU_ARGUMENT_ERROR;
	}
	if (!nicslu->flag[2])
	{
		//printf("NicsLU_Solve_Singular: NICSLU_MATRIX_NOT_FACTORIZED\n");
		return NICSLU_MATRIX_NOT_FACTORIZED;
	}

	/*begin*/
	TimerStart((STimer *)(nicslu->timer));

	n = nicslu->n;
	b = nicslu->rhs;
	up = nicslu->up;
	llen = nicslu->llen;
	ulen = nicslu->ulen;
	ldiag = nicslu->ldiag;
	lu = nicslu->lu_array;
	mc64_scale = nicslu->cfgi[1];
	scale = nicslu->cfgi[2];
	cscale = nicslu->cscale;
	singular = nicslu->singular;

	for (j = 0; j < n; ++j) {
		if (singular[j] == 1) {
			isSingular = 1;
			break;
		}
	}
	if (isSingular == 0) {
		// Matrix is not numerical-singular, just call the regular solver method.
		int ret = NicsLU_Solve(nicslu, rhs);
		TimerStop((STimer *)(nicslu->timer));
		nicslu->stat[3] = TimerGetRuntime((STimer *)(nicslu->timer));
		return ret;
	}

	if (nicslu->cfgi[0] == 0)
	{
		rp = nicslu->row_perm;
		cp = nicslu->col_perm_inv;
		piv = (uint__t *)(nicslu->pivot_inv);
		rows = nicslu->row_scale;
		cols = nicslu->col_scale_perm;

		if (mc64_scale)
		{
			for (j=0; j<n; ++j)
			{
				p = rp[j];
				if (singular[j] == 1) {
					//printf("singular: case 1, j = %d\n", j);
					// Use ref value and no scaling for singular row/column
					b[j] = ref[p];
				} else {
					b[j] = rhs[p] * rows[p];
				}
			}
		}
		else
		{
			for (j=0; j<n; ++j)
			{
				p = rp[j];
				if (singular[j] == 1) {
					//printf("singular: case 2, j = %d\n", j);
					// Use ref value and no scaling for singular row/column
					b[j] = ref[p];
				} else {
					b[j] = rhs[p];
				}
			}
		}

		for (j=0; j<n; ++j)
		{
			if (singular[j] == 1) {
				continue;
			}
			sum = 0.;
			ll = llen[j];
			ul = ulen[j];

			ip = (uint__t *)(((byte__t *)lu) + up[j] + ul*g_sp);
			x = (real__t *)(ip + ll);

			for (p=0; p<ll; ++p)
			{
				sum += x[p] * b[ip[p]];
			}
			//printf("ll = %d, j = %d, sum = %f\n", ll, j, sum);

			b[j] -= sum;
			b[j] /= ldiag[j];
		}

		for (jj=n-1; jj>=0; --jj)
		{
			if (singular[jj] == 1) {
				continue;
			}
			sum = 0.;
			ul = ulen[jj];

			ip = (uint__t *)(((byte__t *)lu) + up[jj]);
			x = (real__t *)(ip + ul);

			for (p=0; p<ul; ++p)
			{
				sum += x[p] * b[ip[p]];
			}
			//printf("ul = %d, j = %d, sum = %f\n", ul, jj, sum);

			b[jj] -= sum;
		}

		if (scale == 1 || scale == 2)
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					if (singular[piv[p]] == 1) {
						//printf("singular: case 3, j = %d\n", piv[p]);
						// No scaling for singular row/column
						rhs[j] = b[piv[p]];
					} else {
						rhs[j] = b[piv[p]] * cols[p] / cscale[p];
					}
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					if (singular[piv[p]] == 1) {
						//printf("singular: case 4, j = %d\n", piv[p]);
						// No scaling for singular row/column
						rhs[j] = b[piv[p]];
					} else {
						rhs[j] = b[piv[p]] / cscale[p];
					}
				}
			}
		}
		else
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = cp[j];
					if (singular[piv[p]] == 1) {
						//printf("singular: case 5, j = %d\n", piv[p]);
						// No scaling for singular row/column
						rhs[j] = b[piv[p]];
					} else {
						rhs[j] = b[piv[p]] * cols[p];
					}
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					rhs[j] = b[piv[cp[j]]];
				}
			}
		}
	}
	else/*CSC*/
	{
		rp = nicslu->col_perm;
		cp = nicslu->row_perm_inv;
		piv = (uint__t *)(nicslu->pivot);
		rows = nicslu->col_scale_perm;
		cols = nicslu->row_scale;

		if (scale == 1 || scale == 2)
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					if (singular[j] == 1) {
						//printf("singular: case 6, j = %d\n", j);
						// Use ref value and no scaling for singular row/column
						b[j] = ref[rp[p]];
					} else {
						b[j] = rhs[rp[p]] * rows[p] / cscale[p];
					}
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					if (singular[j] == 1) {
						//printf("singular: case 7, j = %d\n", j);
						// Use ref value and no scaling for singular row/column
						b[j] = ref[rp[p]];
					} else {
						b[j] = rhs[rp[p]] / cscale[p];
					}
				}
			}
		}
		else
		{
			if (mc64_scale)
			{
				for (j=0; j<n; ++j)
				{
					p = piv[j];
					if (singular[j] == 1) {
						//printf("singular: case 8, j = %d\n", j);
						// Use ref value and no scaling for singular row/column
						b[j] = ref[rp[p]];
					} else {
						b[j] = rhs[rp[p]] * rows[p];
					}
				}
			}
			else
			{
				for (j=0; j<n; ++j)
				{
					b[j] = rhs[rp[piv[j]]];
				}
			}
		}

		for (j=0; j<n; ++j)
		{
			sum = b[j];
			ul = ulen[j];

			ip = (uint__t *)(((byte__t *)lu) + up[j]);
			x = (real__t *)(ip + ul);

			for (p=0; p<ul; ++p)
			{
				if (singular[ip[p]] == 1) {
					continue;
				}
				b[ip[p]] -= x[p] * sum;
			}
		}

		for (jj=n-1; jj>=0; --jj)
		{
			sum = b[jj];
			ll = llen[jj];
			ul = ulen[jj];

			ip = (uint__t *)(((byte__t *)lu) + up[jj] + ul*g_sp);
			x = (real__t *)(ip + ll);

			sum /= ldiag[jj];
			b[jj] = sum;

			for (p=0; p<ll; ++p)
			{
				if (singular[ip[p]] == 1) {
					continue;
				}
				b[ip[p]] -= x[p] * sum;
			}
		}

		if (mc64_scale)
		{
			for (j=0; j<n; ++j)
			{
				p = cp[j];
				if (singular[p] == 1) {
					//printf("singular: case 9, j = %d\n", p);
					rhs[j] = b[p];
				} else {
					rhs[j] = b[p] * cols[j];
				}
			}
		}
		else
		{
			for (j=0; j<n; ++j)
			{
				rhs[j] = b[cp[j]];
			}
		}
	}

	/*finish*/
	TimerStop((STimer *)(nicslu->timer));
	nicslu->stat[3] = TimerGetRuntime((STimer *)(nicslu->timer));

	return NICS_OK;
}
