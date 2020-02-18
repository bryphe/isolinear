module Effect = Effect;
module Store = Store;
module Stream = Stream;
module Updater = Updater;

module Sub: {
  module type Config = {
    type params;
    type msg;

    // State that is carried by the subscription while it is active
    type state;

    let subscriptionName: string;

    let getUniqueId: params => string;

    let init: (~params: params, ~dispatch: msg => unit) => state;
    let update:
      (~params: params, ~state: state, ~dispatch: msg => unit) => state;
    let dispose: (~params: params, ~state: state) => unit;
  };

  type t('msg);

  let batch: list(t('msg)) => t('msg);

  let map: ('a => 'b, t('a)) => t('b);

  module type Sub = {
    type params;
    type msg;

    let create: params => t(msg);
  };

  module Make:
    (ConfigInfo: Config) =>
     Sub with type msg = ConfigInfo.msg and type params = ConfigInfo.params;

  let none: t('msg);
};

module Internal: {
  module Sub = Sub;
  module SubscriptionRunner = SubscriptionRunner;
};