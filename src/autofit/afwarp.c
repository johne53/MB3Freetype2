#include "afwarp.h"

#if 1
static const AF_WarpScore
af_warper_weights[ 64 ] =
{
  35, 20, 20, 20, 15, 12, 10,  5,  2,  1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0, -1, -2, -5, -8,-10,-10,-20,-20,-30,-30,

 -30,-30,-20,-20,-10,-10, -8, -5, -2, -1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  1,  2,  5, 10, 12, 15, 20, 20, 20,
};
#else
static const AF_WarpScore
af_warper_weights[ 64 ] =
{
  30, 20, 10,  5,  4,  4,  3,  2,  1,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0, -1, -2, -2, -5, -5,-10,-10,-15,-20,

 -20,-15,-15,-10,-10, -5, -5, -2, -2, -1,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  4,  5, 10, 20,
};
#endif

static void
af_warper_compute_line_best( AF_Warper     warper,
                             FT_Fixed      scale,
                             FT_Pos        delta,
                             FT_Pos        xx1,
                             FT_Pos        xx2,
                             AF_WarpScore  base_distort,
                             AF_Segment    segments,
                             FT_UInt       num_segments )
{
  FT_Int  idx_min, idx_max, idx0, xx1min, xx1max;
  FT_UInt  nn;
  AF_WarpScore  scores[64];

  for ( nn = 0; nn < 64; nn++ )
    scores[nn] = 0;

  idx0 = xx1 - warper->t1;

  /* compute minimum and maximum indices */
  {
    FT_Pos  xx1min = warper->x1min;
    FT_Pos  xx1max = warper->x1max;
    FT_Pos  w      = xx2 - xx1;

    if ( xx1min + w < warper->x2min )
      xx1min = warper->x2min - (xx2-xx1);

    xx1max = warper->x1max;
    if ( xx1max + w > warper->x2max )
      xx1max = warper->x2max - (xx2-xx1);

    idx_min = xx1min - warper->t1;
    idx_max = xx1max - warper->t1;

    if ( idx_min > idx_max )
    {
      printf( "invalid indices min=%d max=%d xx1=%d xx2=%d x1min=%d x1max=%d x2min=%d x2max=%d\n",
              idx_min, idx_max, xx1, xx2, warper->x1min, warper->x1max, warper->x2min, warper->x2max );
      return;
    }
  }

  for ( nn = 0; nn < num_segments; nn++ )
  {
    FT_Pos  len = segments[nn].max_coord - segments[nn].min_coord;
    FT_Pos  y0  = FT_MulFix( segments[nn].pos, scale ) + delta;
    FT_Pos  y   = y0 + (idx_min - idx0);
    FT_Int  idx;

    for ( idx = idx_min; idx <= idx_max; idx++, y++ )
      scores[idx] += af_warper_weights[ y & 63 ]*len;
  }

  /* find best score */
  {
    FT_Int  idx;

    for ( idx = idx_min; idx <= idx_max; idx++ )
    {
      AF_WarpScore  score = scores[idx];
      AF_WarpScore  distort = base_distort + (idx - idx0);

      if ( score > warper->best_score           ||
           ( score == warper->best_score    &&
             distort < warper->best_distort )   )
      {
        warper->best_score   = score;
        warper->best_distort = distort;
        warper->best_scale   = scale;
        warper->best_delta   = delta + (idx-idx0);
      }
    }
  }
}


FT_LOCAL_DEF( void )
af_warper_compute( AF_Warper      warper,
                   AF_GlyphHints  hints,
                   AF_Dimension   dim,
                   FT_Fixed      *a_scale,
                   FT_Pos        *a_delta )
{
  AF_AxisHints  axis;
  AF_Point      points;
  FT_Fixed      org_scale;
  FT_Pos        org_delta;
  FT_UInt       nn, num_points, num_segments;
  FT_Int        X1, X2;
  FT_Int        w;
  AF_WarpScore  base_distort;
  AF_Segment    segments;

  /* get original scaling transform */
  if ( dim == AF_DIMENSION_VERT )
  {
    org_scale = hints->y_scale;
    org_delta = hints->y_delta;
  }
  else
  {
    org_scale = hints->x_scale;
    org_delta = hints->x_delta;
  }

  warper->best_scale   = org_scale;
  warper->best_delta   = org_delta;
  warper->best_score   = 0;
  warper->best_distort = 0;

  axis         = &hints->axis[dim];
  segments     = axis->segments;
  num_segments = axis->num_segments;
  points       = hints->points;
  num_points   = hints->num_points;

  *a_scale = org_scale;
  *a_delta = org_delta;

  /* get X1 and X2, minimum and maximum in original coordinates */
  if ( axis->num_segments < 1 )
    return;

#if 1
  X1 = X2 = points[0].fx;
  for ( nn = 1; nn < num_points; nn++ )
  {
    FT_Int  X = points[nn].fx;

    if ( X < X1 )
      X1 = X;
    if ( X > X2 )
      X2 = X;
  }
#else
  X1 = X2 = segments[0].pos;
  for ( nn = 1; nn < num_segments; nn++ )
  {
    FT_Int  X = segments[nn].pos;

    if ( X < X1 )
      X1 = X;
    if ( X > X2 )
      X2 = X;
  }
#endif

  if ( X1 >= X2 )
    return;

  warper->x1 = FT_MulFix( X1, org_scale ) + org_delta;
  warper->x2 = FT_MulFix( X2, org_scale ) + org_delta;

  warper->t1 = AF_WARPER_FLOOR(warper->x1);
  warper->t2 = AF_WARPER_CEIL(warper->x2);

  warper->x1min = warper->x1 & ~31;
  warper->x1max = warper->x1min + 32;
  warper->x2min = warper->x2 & ~31;
  warper->x2max = warper->x2min + 32;

  if ( warper->x1max > warper->x2 )
    warper->x1max = warper->x2;

  if ( warper->x2min < warper->x1 )
    warper->x2min = warper->x1;

  warper->w0 = warper->x2 - warper->x1;

  if ( warper->w0 <= 64 )
  {
    warper->x1max = warper->x1;
    warper->x2min = warper->x2;
  }

  warper->wmin = warper->x2min - warper->x1max;
  warper->wmax = warper->x2max - warper->x1min;

  if ( warper->wmin < warper->w0 - 32 )
    warper->wmin = warper->w0 - 32;

  if ( warper->wmax > warper->w0 + 32 )
    warper->wmax = warper->w0 + 32;

  if ( warper->wmin < warper->w0*3/4 )
    warper->wmin = warper->w0*3/4;

  if ( warper->wmax > warper->w0*5/4 )
    warper->wmax = warper->w0*5/4;


  /* warper->wmin = warper->wmax = warper->w0; */

  for ( w = warper->wmin; w <= warper->wmax; w++ )
  {
    FT_Int   line = w - warper->wmin;
    FT_Fixed  new_scale;
    FT_Pos    new_delta;
    FT_Pos    xx1, xx2;

    xx1 = warper->x1;
    xx2 = warper->x2;
    if ( w >= warper->w0 )
    {
      xx1 -= (w-warper->w0);
      if ( xx1 < warper->x1min )
      {
        xx2 += warper->x1min - xx1;
        xx1  = warper->x1min;
      }
    }
    else
    {
      xx1 -= (w-warper->w0);
      if ( xx1 > warper->x1max )
      {
        xx2 -= (xx1-warper->x1max);
        xx1  = warper->x1max;
      }
    }

    if ( xx1 < warper->x1 )
      base_distort = warper->x1 - xx1;
    else
      base_distort = xx1 - warper->x1;

    if ( xx2 < warper->x2 )
      base_distort += warper->x2 - xx2;
    else
      base_distort += xx2 - warper->x2;

    base_distort *= 10;

    new_scale = org_scale + FT_DivFix( w - warper->w0, X2-X1 );
    new_delta = xx1 - FT_MulFix( X1, new_scale );

    af_warper_compute_line_best( warper, new_scale, new_delta, xx1, xx2,
                                 base_distort,
                                 segments, num_segments );
  }

  *a_scale = warper->best_scale;
  *a_delta = warper->best_delta;
}
