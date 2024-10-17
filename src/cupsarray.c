/*
 * "cupsarray.c 2021-05-17 15:55:05
 *  
 *  cups array routines for TSC Printer Driver
 *  
 *  Copyright (c) 2005, by TSC Printronix Auto ID .
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at

 *      http://www.apache.org/licenses/LICENSE-2.0

 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *	
 *
 */

#include "config.h"
#include "common.h"
#include "debug.h"

#include "cupsinc/cups.h"
#include "cupsinc/array.h"

#include "mycups.h"

/*
 * Limits...
 */

#define _CUPS_MAXSAVE	32		/**** Maximum number of saves ****/

/*
 * Types and structures...
 */

struct _cups_array_s			/**** CUPS array structure ****/
{
 /*
  * The current implementation uses an insertion sort into an array of
  * sorted pointers.  We leave the array type private/opaque so that we
  * can change the underlying implementation without affecting the users
  * of this API.
  */

  int			num_elements,	/* Number of array elements */
			alloc_elements,	/* Allocated array elements */
			current,	/* Current element */
			insert,		/* Last inserted element */
			unique,		/* Are all elements unique? */
			num_saved,	/* Number of saved elements */
			saved[_CUPS_MAXSAVE];
					/* Saved elements */
  void			**elements;	/* Array elements */
  cups_array_func_t	compare;	/* Element comparison function */
  void			*data;		/* User data passed to compare */
  cups_ahash_func_t	hashfunc;	/* Hash function */
  int			hashsize,	/* Size of hash */
			*hash;		/* Hash array */
};


/*
 * Local functions...
 */

static int	cups_array_add(cups_array_t *a, void *e, int insert);
static int	cups_array_find(cups_array_t *a, void *e, int prev, int *rdiff);

/*
 * 'cupsArrayAdd()' - Add an element to the array.
 *
 * When adding an element to a sorted array, non-unique elements are
 * appended at the end of the run.  For unsorted arrays, the element
 * is inserted at the end of the array.
 */

int					/* O - 1 on success, 0 on failure */
my_cupsArrayAdd(cups_array_t *a,		/* I - Array */
             void         *e)		/* I - Element */
{
//  DEBUG_printf(("cupsArrayAdd(a=%p, e=%p)\n", a, e));

 /*
  * Range check input...
  */

  if (!a || !e)
  {
//    DEBUG_puts("cupsArrayAdd: returning 0");
    return (0);
  }

 /*
  * Append the element...
  */

  return (cups_array_add(a, e, 0));
}

/*
 * 'cupsArrayClear()' - Clear the array.
 */

void
my_cupsArrayClear(cups_array_t *a)		/* I - Array */
{
 /*
  * Range check input...
  */

  if (!a)
    return;

 /*
  * Set the number of elements to 0; we don't actually free the memory
  * here - that is done in cupsArrayDelete()...
  */

  a->num_elements = 0;
  a->current      = -1;
  a->insert       = -1;
  a->unique       = 1;
  a->num_saved    = 0;
}

/*
 * 'cupsArrayCount()' - Get the number of elements in the array.
 */

int					/* O - Number of elements */
my_cupsArrayCount(cups_array_t *a)		/* I - Array */
{
 /*
  * Range check input...
  */

  if (!a)
    return (0);

 /*
  * Return the number of elements...
  */

  return (a->num_elements);
}

/*
 * 'cupsArrayCurrent()' - Return the current element in the array.
 */

void *					/* O - Element */
my_cupsArrayCurrent(cups_array_t *a)	/* I - Array */
{
 /*
  * Range check input...
  */

  if (!a)
    return (NULL);

 /*
  * Return the current element...
  */

  if (a->current >= 0 && a->current < a->num_elements)
    return (a->elements[a->current]);
  else
    return (NULL);
}

/*
 * 'cupsArrayFirst()' - Get the first element in the array.
 */

void *					/* O - First element or NULL */
my_cupsArrayFirst(cups_array_t *a)		/* I - Array */
{
 /*
  * Range check input...
  */

  if (!a)
    return (NULL);

 /*
  * Return the first element...
  */

  a->current = 0;

  return (my_cupsArrayCurrent(a));
}

/*
 * 'cupsArrayIndex()' - Get the N-th element in the array.
 */

void *					/* O - N-th element or NULL */
my_cupsArrayIndex(cups_array_t *a,		/* I - Array */
               int          n)		/* I - Index into array, starting at 0 */
{
  if (!a)
    return (NULL);

  a->current = n;

  return (my_cupsArrayCurrent(a));
}

/*
 * 'cupsArrayLast()' - Get the last element in the array.
 */

void *					/* O - Last element or NULL */
my_cupsArrayLast(cups_array_t *a)		/* I - Array */
{
 /*
  * Range check input...
  */

  if (!a)
    return (NULL);

 /*
  * Return the last element...
  */

  a->current = a->num_elements - 1;

  return (my_cupsArrayCurrent(a));
}

/*
 * 'cupsArrayNew()' - Create a new array.
 */

cups_array_t *				/* O - Array */
my_cupsArrayNew(cups_array_func_t f,	/* I - Comparison function */
             void              *d)	/* I - User data */
{
  return (my_cupsArrayNew2(f, d, 0, 0));
}

/*
 * 'cupsArrayNew2()' - Create a new array with hash.
 *
 * @since CUPS 1.3@
 */

cups_array_t *				/* O - Array */
my_cupsArrayNew2(cups_array_func_t  f,	/* I - Comparison function */
              void               *d,	/* I - User data */
              cups_ahash_func_t  h,	/* I - Hash function*/
	      int                hsize)	/* I - Hash size */
{
  cups_array_t	*a;			/* Array  */


 /*
  * Allocate memory for the array...
  */

  a = calloc(1, sizeof(cups_array_t));
  if (!a)
    return (NULL);

  a->compare   = f;
  a->data      = d;
  a->current   = -1;
  a->insert    = -1;
  a->num_saved = 0;
  a->unique    = 1;

  if (hsize > 0 && h)
  {
    a->hashfunc  = h;
    a->hashsize  = hsize;
    a->hash      = malloc(hsize * sizeof(int));

    if (!a->hash)
    {
      free(a);
      return (NULL);
    }

    memset(a->hash, -1, hsize * sizeof(int));
  }

  return (a);
}

/*
 * 'cups_array_add()' - Insert or append an element to the array...
 */

static int				/* O - 1 on success, 0 on failure */
cups_array_add(cups_array_t *a,		/* I - Array */
               void         *e,		/* I - Element to add */
	       int          insert)	/* I - 1 = insert, 0 = append */
{
  int	i,				/* Looping var */
	current,			/* Current element */
	diff;				/* Comparison with current element */


//  DEBUG_printf(("cups_array_add(a=%p, e=%p, insert=%d)\n", a, e, insert));

 /*
  * Verify we have room for the new element...
  */

  if (a->num_elements >= a->alloc_elements)
  {
   /*
    * Allocate additional elements; start with 16 elements, then
    * double the size until 1024 elements, then add 1024 elements
    * thereafter...
    */

    void	**temp;			/* New array elements */
    int		count;			/* New allocation count */


    if (a->alloc_elements == 0)
    {
      count = 16;
      temp  = malloc(count * sizeof(void *));
    }
    else
    {
      if (a->alloc_elements < 1024)
        count = a->alloc_elements * 2;
      else
        count = a->alloc_elements + 1024;

      temp = realloc(a->elements, count * sizeof(void *));
    }

//    DEBUG_printf(("cups_array_add: count=%d\n", count));

    if (!temp)
    {
//      DEBUG_puts("cupsAddAdd: allocation failed, returning 0");
      return (0);
    }

    a->alloc_elements = count;
    a->elements       = temp;
  }

 /*
  * Find the insertion point for the new element; if there is no
  * compare function or elements, just add it to the beginning or end...
  */

  if (!a->num_elements || !a->compare)
  {
   /*
    * No elements or comparison function, insert/append as needed...
    */

    if (insert)
      current = 0;			/* Insert at beginning */
    else
      current = a->num_elements;	/* Append to the end */
  }
  else
  {
   /*
    * Do a binary search for the insertion point...
    */

    current = cups_array_find(a, e, a->insert, &diff);

    if (diff > 0)
    {
     /*
      * Insert after the current element...
      */

      current ++;
    }
    else if (!diff)
    {
     /*
      * Compared equal, make sure we add to the begining or end of
      * the current run of equal elements...
      */

      a->unique = 0;

      if (insert)
      {
       /*
        * Insert at beginning of run...
	*/

	while (current > 0 && !(*(a->compare))(e, a->elements[current - 1],
                                               a->data))
          current --;
      }
      else
      {
       /*
        * Append at end of run...
	*/

	do
	{
          current ++;
	}
	while (current < a->num_elements &&
               !(*(a->compare))(e, a->elements[current], a->data));
      }
    }
  }

 /*
  * Insert or append the element...
  */

  if (current < a->num_elements)
  {
   /*
    * Shift other elements to the right...
    */

    memmove(a->elements + current + 1, a->elements + current,
            (a->num_elements - current) * sizeof(void *));

    if (a->current >= current)
      a->current ++;

    for (i = 0; i < a->num_saved; i ++)
      if (a->saved[i] >= current)
	a->saved[i] ++;

//    DEBUG_printf(("cups_array_add: insert element at index %d...\n", current));
  }
//#ifdef DEBUG
//  else
//    printf("cups_array_add: append element at %d...\n", current);
//#endif /* DEBUG */

  a->elements[current] = e;
  a->num_elements ++;
  a->insert = current;

//#ifdef DEBUG
//  for (current = 0; current < a->num_elements; current ++)
//    printf("cups_array_add: a->elements[%d]=%p\n", current, a->elements[current]);
//#endif /* DEBUG */

//  DEBUG_puts("cups_array_add: returning 1");

  return (1);
}


/*
 * 'cups_array_find()' - Find an element in the array...
 */

static int				/* O - Index of match */
cups_array_find(cups_array_t *a,	/* I - Array */
        	void         *e,	/* I - Element */
		int          prev,	/* I - Previous index */
		int          *rdiff)	/* O - Difference of match */
{
  int	left,				/* Left side of search */
	right,				/* Right side of search */
	current,			/* Current element */
	diff;				/* Comparison with current element */


//  DEBUG_printf(("cups_array_find(a=%p, e=%p, prev=%d, rdiff=%p)\n", a, e, prev,
//                rdiff));

  if (a->compare)
  {
   /*
    * Do a binary search for the element...
    */

//    DEBUG_puts("cups_array_find: binary search");

    if (prev >= 0 && prev < a->num_elements)
    {
     /*
      * Start search on either side of previous...
      */

      if ((diff = (*(a->compare))(e, a->elements[prev], a->data)) == 0 ||
          (diff < 0 && prev == 0) ||
	  (diff > 0 && prev == (a->num_elements - 1)))
      {
       /*
        * Exact or edge match, return it!
	*/

//        DEBUG_printf(("cups_array_find: Returning %d, diff=%d\n", prev, diff));

	*rdiff = diff;

	return (prev);
      }
      else if (diff < 0)
      {
       /*
        * Start with previous on right side...
	*/

	left  = 0;
	right = prev;
      }
      else
      {
       /*
        * Start wih previous on left side...
	*/

        left  = prev;
	right = a->num_elements - 1;
      }
    }
    else
    {
     /*
      * Start search in the middle...
      */

      left  = 0;
      right = a->num_elements - 1;
    }

    do
    {
      current = (left + right) / 2;
      diff    = (*(a->compare))(e, a->elements[current], a->data);

//      DEBUG_printf(("cups_array_find: left=%d, right=%d, current=%d, diff=%d\n",
//                    left, right, current, diff));

      if (diff == 0)
	break;
      else if (diff < 0)
	right = current;
      else
	left = current;
    }
    while ((right - left) > 1);

    if (diff != 0)
    {
     /*
      * Check the last 1 or 2 elements...
      */

      if ((diff = (*(a->compare))(e, a->elements[left], a->data)) <= 0)
        current = left;
      else
      {
        diff    = (*(a->compare))(e, a->elements[right], a->data);
        current = right;
      }
    }
  }
  else
  {
   /*
    * Do a linear pointer search...
    */

//    DEBUG_puts("cups_array_find: linear search");

    diff = 1;

    for (current = 0; current < a->num_elements; current ++)
      if (a->elements[current] == e)
      {
        diff = 0;
        break;
      }
  }

 /*
  * Return the closest element and the difference...
  */

//  DEBUG_printf(("cups_array_find: Returning %d, diff=%d\n", current, diff));

  *rdiff = diff;

  return (current);
}
