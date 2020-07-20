let runSymbolic = (inputs: RenderTypes.ShapeRenderer.Combined.inputs) => {
  let graph = MathJsParser.fromString(inputs.guesstimatorString);
  graph
  |> E.R.bind(_, g =>
       ExpressionTree.toShape(
         inputs.symbolicInputs.length,
         {
           sampleCount:
             inputs.samplingInputs.sampleCount |> E.O.default(10000),
           outputXYPoints:
             inputs.samplingInputs.outputXYPoints |> E.O.default(10000),
           kernelWidth: inputs.samplingInputs.kernelWidth,
         },
         g,
       )
       |> E.R.fmap(RenderTypes.ShapeRenderer.Symbolic.make(g))
     );
};

let run = (inputs: RenderTypes.ShapeRenderer.Combined.inputs) => {
  runSymbolic(inputs);
};
