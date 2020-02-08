module InputTypes = {
  type t =
    | Year(float)
    | SingleChoice(string)
    | FloatPoint(float)
    | FloatCdf(string);

  type withName = (string, t);
  let withoutName = ((_, t): withName) => t;
  type withNames = list(withName);
  let getName = (r: withNames, name) =>
    r->Belt.List.getBy(((n, _)) => n == name);
  let to_string = (t: t) => {
    switch (t) {
    | SingleChoice(r) => r
    | FloatCdf(r) => r
    | Year(r) => r |> Js.Float.toFixed
    | FloatPoint(r) => r |> Js.Float.toFixed
    };
  };
};

module IOTypes = {
  type singleChoice = {
    options: list((string, string)),
    default: option(string),
  };
  type floatPoint = {validatations: list(float => bool)};
  type withDefaultMinMax('a) = {
    default: option('a),
    min: option('a),
    max: option('a),
  };
};

module Input = {
  type parameterType =
    | Year(IOTypes.withDefaultMinMax(float))
    | SingleChoice(IOTypes.singleChoice)
    | FloatPoint;

  let toInput = (p: parameterType) =>
    switch (p) {
    | Year(r) => r.default->Belt.Option.map(p => InputTypes.Year(p))
    | SingleChoice(r) =>
      r.default->Belt.Option.map(p => InputTypes.SingleChoice(p))
    | FloatPoint => None
    };

  type parameter = {
    id: string,
    name: string,
    parameterType,
  };

  let currentYear = {
    id: "currentyear",
    name: "Current Year",
    parameterType: FloatPoint,
  };

  let make = (~name, ~parameterType, ~id=name, ()) => {
    id,
    name,
    parameterType,
  };

  module Form = {
    let handleChange = (handleChange, event) =>
      handleChange(ReactEvent.Form.target(event)##value);
    [@react.component]
    let make =
        (~parameter: parameter, ~value: option(InputTypes.t), ~onChange) => {
      switch (parameter.parameterType, value) {
      | (Year(_), Some(Year(r))) =>
        <input
          type_="number"
          value={r |> Js.Float.toString}
          onChange={handleChange(r =>
            switch (Js.Float.fromString(r)) {
            | r => onChange(_ => Some(InputTypes.Year(r)))
            }
          )}
        />
      | (FloatPoint, Some(FloatPoint(r))) =>
        <input type_="number" value={r |> Js.Float.toString} />
      | (Year(_), _)
      | (FloatPoint, _) => <input type_="number" value="" />
      | (SingleChoice(_), _) =>
        <div> {"Single Choice" |> ReasonReact.string} </div>
      };
    };
  };
};

module Output = {
  type parameterType =
    | Year
    | SingleChoice(IOTypes.singleChoice)
    | FloatPoint
    | FloatCdf;

  type parameter = {
    id: string,
    name: string,
    parameterType,
  };

  type outputConfig =
    | Single;

  let make = (~name, ~parameterType, ~id=name, ()) => {
    id,
    name,
    parameterType,
  };
};

type model = {
  name: string,
  author: string,
  assumptions: list(Input.parameter),
  inputs: list(Input.parameter),
  outputs: list(Output.parameter),
  outputConfig: Output.outputConfig,
};

let gatherInputs = (m: model, a: list(InputTypes.withName)) => {
  let getItem = (p: Input.parameter) => InputTypes.getName(a, p.id);
  [
    m.assumptions |> List.map(getItem),
    m.inputs |> List.map(getItem),
    [InputTypes.getName(a, "output")],
  ]
  |> List.flatten;
};

module MS = Belt.Map.String;

type modelMaps = {
  assumptions: MS.t((Input.parameter, option(InputTypes.t))),
  inputs: MS.t((Input.parameter, option(InputTypes.t))),
  output: (Output.parameter, option(InputTypes.t)),
};

let toMaps = (m: model): modelMaps => {
  assumptions:
    MS.fromArray(
      m.assumptions
      |> List.map((r: Input.parameter) =>
           (r.id, (r, Input.toInput(r.parameterType)))
         )
      |> Array.of_list,
    ),
  inputs:
    MS.fromArray(
      m.inputs
      |> List.map((r: Input.parameter) =>
           (r.id, (r, Input.toInput(r.parameterType)))
         )
      |> Array.of_list,
    ),
  output: (Output.make(~name="Payouts", ~parameterType=FloatCdf, ()), None),
};

type modelParams = {
  assumptions: list(option(InputTypes.t)),
  inputs: list(option(InputTypes.t)),
  output: option(InputTypes.t),
};

let response = (m: model, a: list(InputTypes.withName)) => {
  let getItem = (p: Input.parameter) =>
    InputTypes.getName(a, p.id)->Belt.Option.map(InputTypes.withoutName);
  {
    assumptions: m.assumptions |> List.map(getItem),
    inputs: m.inputs |> List.map(getItem),
    output:
      InputTypes.getName(a, "output")
      ->Belt.Option.map(InputTypes.withoutName),
  };
};