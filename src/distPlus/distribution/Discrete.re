open Distributions;

type t = DistTypes.discreteShape;

let make = (xyShape, integralSumCache, integralCache): t => {xyShape, integralSumCache, integralCache};
let shapeMap = (fn, {xyShape, integralSumCache, integralCache}: t): t => {
  xyShape: fn(xyShape),
  integralSumCache,
  integralCache
};
let getShape = (t: t) => t.xyShape;
let oShapeMap = (fn, {xyShape, integralSumCache, integralCache}: t): option(t) =>
  fn(xyShape) |> E.O.fmap(make(_, integralSumCache, integralCache));

let empty: t = {xyShape: XYShape.T.empty, integralSumCache: Some(0.0), integralCache: None};
let shapeFn = (fn, t: t) => t |> getShape |> fn;

let lastY = (t: t) => t |> getShape |> XYShape.T.lastY;

let combinePointwise =
    (
      ~integralSumCachesFn = (_, _) => None,
      ~integralCachesFn: (DistTypes.continuousShape, DistTypes.continuousShape) => option(DistTypes.continuousShape) = (_, _) => None,
      fn,
      t1: DistTypes.discreteShape,
      t2: DistTypes.discreteShape,
    )
    : DistTypes.discreteShape => {
  let combinedIntegralSum =
    Common.combineIntegralSums(
      integralSumCachesFn,
      t1.integralSumCache,
      t2.integralSumCache,
    );

  // TODO: does it ever make sense to pointwise combine the integrals here?
  // It could be done for pointwise additions, but is that ever needed?

  make(
    XYShape.PointwiseCombination.combine(
      (+.),
      XYShape.XtoY.discreteInterpolator,
      XYShape.XtoY.discreteInterpolator,
      t1.xyShape,
      t2.xyShape,
    ),
    combinedIntegralSum,
    None,
  );
};

let reduce =
  (~integralSumCachesFn=(_, _) => None,
   ~integralCachesFn=(_, _) => None,
   fn, discreteShapes)
  : DistTypes.discreteShape =>
  discreteShapes
  |> E.A.fold_left(combinePointwise(~integralSumCachesFn, ~integralCachesFn, fn), empty);

let updateIntegralSumCache = (integralSumCache, t: t): t => {
  ...t,
  integralSumCache,
};

let updateIntegralCache = (integralCache, t: t): t => {
  ...t,
  integralCache,
};

/* This multiples all of the data points together and creates a new discrete distribution from the results.
   Data points at the same xs get added together. It may be a good idea to downsample t1 and t2 before and/or the result after. */
let combineAlgebraically =
    (op: ExpressionTypes.algebraicOperation, t1: t, t2: t): t => {
  let t1s = t1 |> getShape;
  let t2s = t2 |> getShape;
  let t1n = t1s |> XYShape.T.length;
  let t2n = t2s |> XYShape.T.length;

  let combinedIntegralSum =
    Common.combineIntegralSums(
      (s1, s2) => Some(s1 *. s2),
      t1.integralSumCache,
      t2.integralSumCache,
    );

  let fn = Operation.Algebraic.toFn(op);
  let xToYMap = E.FloatFloatMap.empty();

  for (i in 0 to t1n - 1) {
    for (j in 0 to t2n - 1) {
      let x = fn(t1s.xs[i], t2s.xs[j]);
      let cv = xToYMap |> E.FloatFloatMap.get(x) |> E.O.default(0.);
      let my = t1s.ys[i] *. t2s.ys[j];
      let _ = Belt.MutableMap.set(xToYMap, x, cv +. my);
      ();
    };
  };

  let rxys = xToYMap |> E.FloatFloatMap.toArray |> XYShape.Zipped.sortByX;

  let combinedShape = XYShape.T.fromZippedArray(rxys);

  make(combinedShape, combinedIntegralSum, None);
};

let mapY = (~integralSumCacheFn=_ => None,
            ~integralCacheFn=_ => None,
            fn, t: t) => {
  let yMapFn = shapeMap(XYShape.T.mapY(fn));

  t
  |> yMapFn
  |> updateIntegralSumCache(E.O.bind(t.integralSumCache, integralSumCacheFn))
  |> updateIntegralCache(E.O.bind(t.integralCache, integralCacheFn));
};


let scaleBy = (~scale=1.0, t: t): t => {
  let scaledIntegralSumCache = E.O.bind(t.integralSumCache, v => Some(scale *. v));
  let scaledIntegralCache = E.O.bind(t.integralCache, v => Some(Continuous.scaleBy(~scale, v)));

  t
  |> mapY((r: float) => r *. scale)
  |> updateIntegralSumCache(scaledIntegralSumCache)
  |> updateIntegralCache(scaledIntegralCache)
};

module T =
  Dist({
    type t = DistTypes.discreteShape;
    type integral = DistTypes.continuousShape;
    let integral = (t) =>
      if (t |> getShape |> XYShape.T.isEmpty) {
        Continuous.make(
          `Stepwise,
          {xs: [|neg_infinity|], ys: [|0.0|]},
          None,
          None,
        );
      } else {
        switch (t.integralCache) {
        | Some(c) => c
        | None => {
            let ts = getShape(t);
            // The first xy of this integral should always be the zero, to ensure nice plotting
            let firstX = ts |> XYShape.T.minX;
            let prependedZeroPoint: XYShape.T.t = {xs: [|firstX -. epsilon_float|], ys: [|0.|]};
            let integralShape =
              ts
              |> XYShape.T.concat(prependedZeroPoint)
              |> XYShape.T.accumulateYs((+.));

            Continuous.make(`Stepwise, integralShape, None, None);
          }
        };
      };

    let integralEndY = (t: t) =>
      t.integralSumCache
      |> E.O.default(t |> integral |> Continuous.lastY);
    let minX = shapeFn(XYShape.T.minX);
    let maxX = shapeFn(XYShape.T.maxX);
    let toDiscreteProbabilityMassFraction = _ => 1.0;
    let mapY = mapY;
    let updateIntegralCache = updateIntegralCache;
    let toShape = (t: t): DistTypes.shape => Discrete(t);
    let toContinuous = _ => None;
    let toDiscrete = t => Some(t);

    let normalize = (t: t): t => {
      t
      |> scaleBy(~scale=1. /. integralEndY(t))
      |> updateIntegralSumCache(Some(1.0));
    };

    let normalizedToContinuous = _ => None;
    let normalizedToDiscrete = t => Some(t); // TODO: this should be normalized!

    let downsample = (i, t: t): t => {
      // It's not clear how to downsample a set of discrete points in a meaningful way.
      // The best we can do is to clip off the smallest values.
      let currentLength = t |> getShape |> XYShape.T.length;

      if (i < currentLength && i >= 1 && currentLength > 1) {
        let clippedShape =
          t
          |> getShape
          |> XYShape.T.zip
          |> XYShape.Zipped.sortByY
          |> Belt.Array.reverse
          |> Belt.Array.slice(_, ~offset=0, ~len=i)
          |> XYShape.Zipped.sortByX
          |> XYShape.T.fromZippedArray;

        make(clippedShape, None, None); // if someone needs the sum, they'll have to recompute it
      } else {
        t;
      };
    };

    let truncate =
        (leftCutoff: option(float), rightCutoff: option(float), t: t): t => {
      let truncatedShape =
        t
        |> getShape
        |> XYShape.T.zip
        |> XYShape.Zipped.filterByX(x =>
             x >= E.O.default(neg_infinity, leftCutoff)
             || x <= E.O.default(infinity, rightCutoff)
           )
        |> XYShape.T.fromZippedArray;

      make(truncatedShape, None, None);
    };

    let xToY = (f, t) =>
      t
      |> getShape
      |> XYShape.XtoY.stepwiseIfAtX(f)
      |> E.O.default(0.0)
      |> DistTypes.MixedPoint.makeDiscrete;

    let integralXtoY = (f, t) =>
      t |> integral |> Continuous.getShape |> XYShape.XtoY.linear(f);

    let integralYtoX = (f, t) =>
      t |> integral |> Continuous.getShape |> XYShape.YtoX.linear(f);

    let mean = (t: t): float => {
      let s = getShape(t);
      E.A.reducei(s.xs, 0.0, (acc, x, i) => acc +. x *. s.ys[i]);
    };
    let variance = (t: t): float => {
      let getMeanOfSquares = t =>
        t |> shapeMap(XYShape.Analysis.squareXYShape) |> mean;
      XYShape.Analysis.getVarianceDangerously(t, mean, getMeanOfSquares);
    };
  });