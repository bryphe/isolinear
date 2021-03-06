open TestFramework;

module Sub = Isolinear.Sub;
module SubscriptionRunner = Isolinear.Testing.SubscriptionRunner;

type testState =
  | Init(int)
  | Update(int)
  | Dispose(int);

type testState2 =
  | Init2(int)
  | Update2(int)
  | Dispose2(int);

let map =
  fun
  | Init(v) => Init2(v)
  | Update(v) => Update2(v)
  | Dispose(v) => Dispose2(v);

let disposeState: ref(list(testState)) = ref([]);

module Runner =
  SubscriptionRunner.Make({
    type msg = testState;
  });

module Runner2 =
  SubscriptionRunner.Make({
    type msg = testState2;
  });

module TestSubscription =
  Sub.Make({
    type params = int;
    type msg = testState;

    type state = list(testState);

    let name = "TestSubscription";

    let id = params => params |> string_of_int;

    let init = (~params, ~dispatch) => {
      dispatch(Init(params));
      [Init(params)];
    };

    let update = (~params, ~state, ~dispatch) => {
      let newState = [Update(params), ...state];
      dispatch(Update(params));
      newState;
    };

    let dispose = (~params as _, ~state as _) => {
      ();
    };
  });

let globalDispatch = ref(None);

module SubscriptionThatHoldsOnToDispatch =
  Sub.Make({
    type params = int;
    type msg = testState;

    type state = unit;

    let name = "SubscriptionThatHoldsOnToDispatch";

    let id = params => params |> string_of_int;

    let init = (~params, ~dispatch) => {
      globalDispatch := Some(dispatch);
      ();
    };

    let update = (~params as _, ~state as _, ~dispatch as _) => {
      ();
    };

    let dispose = (~params as _, ~state as _) => {
      ();
    };
  });

describe("SubscriptionRunner", ({describe, _}) => {
  describe("dispose", ({test, _}) => {
    test(
      "dispatch called after subscription is gone is a no-op", ({expect, _}) => {
      let actions: ref(list(testState)) = ref([]);
      let dispatch = action => {
        actions := [action, ...actions^];
      };
      let sub = SubscriptionThatHoldsOnToDispatch.create(1);
      let runner = Runner.run(~dispatch, ~sub, Runner.empty);

      globalDispatch^ |> Option.iter(dispatch => dispatch(Init(1)));

      // Not disposed... should've gotten an action!
      expect.equal(actions^, [Init(1)]);

      actions := [];

      // Dispose of sub
      let _ = Runner.run(~dispatch, ~sub=Isolinear.Sub.none, runner);

      globalDispatch^ |> Option.iter(dispatch => dispatch(Init(2)));

      expect.equal(actions^, []);
    })
  });

  describe("subscribe", ({test, _}) => {
    test("init is called", ({expect, _}) => {
      let lastAction: ref(option(testState)) = ref(None);
      let dispatch = action => {
        lastAction := Some(action);
      };
      let sub = TestSubscription.create(1);
      let _ = Runner.run(~dispatch, ~sub, Runner.empty);

      expect.equal(lastAction^, Some(Init(1)));
    });

    test("update is called", ({expect, _}) => {
      let lastAction: ref(option(testState)) = ref(None);
      let dispatch = action => {
        lastAction := Some(action);
      };
      let sub = TestSubscription.create(1);
      let newState = Runner.run(~dispatch, ~sub, Runner.empty);
      let _ = Runner.run(~dispatch, ~sub, newState);

      expect.equal(lastAction^, Some(Update(1)));
    });
  });

  describe("batched subscriptions", ({test, _}) => {
    test("init called for both", ({expect, _}) => {
      let allActions: ref(list(testState)) = ref([]);
      let dispatch = action => {
        allActions := [action, ...allActions^];
      };
      let sub1 = TestSubscription.create(1);
      let sub2 = TestSubscription.create(2);

      let subs = Sub.batch([sub1, sub2]);
      let state = Runner.run(~dispatch, ~sub=subs, Runner.empty);

      expect.equal(allActions^ |> List.rev, [Init(1), Init(2)]);

      // Remove one action
      let subs = Sub.batch([sub2]);
      let state = Runner.run(~dispatch, ~sub=subs, state);

      expect.equal(
        allActions^ |> List.rev,
        [Init(1), Init(2), Update(2)],
      );

      // Bring back action
      let subs = Sub.batch([sub1, sub2]);
      let _state = Runner.run(~dispatch, ~sub=subs, state);

      expect.equal(
        allActions^ |> List.rev,
        [Init(1), Init(2), Update(2), Init(1), Update(2)],
      );
    })
  });

  describe("mapped subscriptions", ({test, _}) => {
    test("init called for both", ({expect, _}) => {
      let allActions: ref(list(testState2)) = ref([]);
      let dispatch = action => {
        allActions := [action, ...allActions^];
      };
      let sub1: Sub.t(testState2) =
        TestSubscription.create(1) |> Sub.map(map);

      let state = Runner2.run(~dispatch, ~sub=sub1, Runner2.empty);

      expect.equal(allActions^ |> List.rev, [Init2(1)]);

      let _: Runner2.t = Runner2.run(~dispatch, ~sub=sub1, state);

      expect.equal(allActions^ |> List.rev, [Init2(1), Update2(1)]);
    })
  });
});
