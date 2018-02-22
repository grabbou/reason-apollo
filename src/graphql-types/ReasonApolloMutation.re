open Js.Result;

module type InternalConfig = {
  let apolloClient: ApolloClient.generatedApolloClient;
};

external castPromiseError : Js.Promise.error => exn = "%identity";

module MutationFactory = (InternalConfig: InternalConfig) => {
  external cast :
    string =>
    {
      .
      "data": Js.Json.t,
      "loading": bool
    } =
    "%identity";
  [@bs.module] external gql : ReasonApolloTypes.gql = "graphql-tag";
  type state =
    | NotCalled
    | Loading
    | Loaded(Js.Json.t)
    | Failed(Js.Promise.error);
  type action = Js.Result.t(Js.Json.t, Js.Promise.error);
  let sendMutation = (~mutation, ~send) =>
    Js.Promise.(
      resolve(
        InternalConfig.apolloClient##mutate({
          "mutation": [@bs] gql(mutation##query),
          "variables": mutation##variables
        })
      )
      |> then_(result => {
           let typedResult = cast(result)##data;
           send(Ok(typedResult));
           resolve(Ok(typedResult));
         })
      |> catch(error => {
           send(Error(error));
           resolve(Error(error));
         })
    );
  let component = ReasonReact.reducerComponent("ReasonApollo");
  let make = children => {
    ...component,
    initialState: () => NotCalled,
    reducer: (action, _state) =>
      switch action {
      | Ok(typedResult) => ReasonReact.Update(Loaded(typedResult))
      | Error(error) => ReasonReact.Update(Failed(error))
      },
    render: ({send, state}) => {
      let mutate = mutationFactory =>
        sendMutation(~mutation=mutationFactory, ~send);
      children(mutate, state);
    }
  };
};