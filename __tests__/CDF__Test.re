open Jest;
open Expect;

exception ShapeWrong(string);
describe("CDF", () => {
  test("raise - w/o order", () => {
    expect(() => {
      module Cdf =
        CDF.Make({
          let shape: DistTypes.xyShape = {
            xs: [|10., 4., 8.|],
            ys: [|8., 9., 2.|],
          };
        });
      ();
    })
    |> toThrow
  });
  test("raise - with order", () => {
    expect(() => {
      module Cdf =
        CDF.Make({
          let shape: DistTypes.xyShape = {
            xs: [|1., 4., 8.|],
            ys: [|8., 9., 2.|],
          };
        });
      ();
    })
    |> not_
    |> toThrow
  });
  test("order#1", () => {
    let a = CDF.order({xs: [|1., 4., 8.|], ys: [|8., 9., 2.|]});
    let b: DistTypes.xyShape = {xs: [|1., 4., 8.|], ys: [|8., 9., 2.|]};
    expect(a) |> toEqual(b);
  });
  test("order#2", () => {
    let a = CDF.order({xs: [|10., 5., 12.|], ys: [|8., 9., 2.|]});
    let b: DistTypes.xyShape = {xs: [|5., 10., 12.|], ys: [|9., 8., 2.|]};
    expect(a) |> toEqual(b);
  });
  test("minX", () => {
    module Dist =
      CDF.Make({
        let shape = CDF.order({xs: [|20., 4., 8.|], ys: [|8., 9., 2.|]});
      });
    expect(Dist.minX()) |> toEqual(4.);
  });
  test("maxX", () => {
    module Dist =
      CDF.Make({
        let shape = CDF.order({xs: [|20., 4., 8.|], ys: [|8., 9., 2.|]});
      });
    expect(Dist.maxX()) |> toEqual(20.);
  });
  test("findY#1", () => {
    module Dist =
      CDF.Make({
        let shape = CDF.order({xs: [|1., 2., 3.|], ys: [|5., 6., 7.|]});
      });
    expect(Dist.findY(1.)) |> toEqual(5.);
  });
  test("findY#2", () => {
    module Dist =
      CDF.Make({
        let shape = CDF.order({xs: [|1., 2., 3.|], ys: [|5., 6., 7.|]});
      });
    expect(Dist.findY(1.5)) |> toEqual(5.5);
  });
});