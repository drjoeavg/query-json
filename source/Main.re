open Ast;
open Tokenizer;

let indent = n => String.make(n * 2, ' ');

let positionToString = (start, end_) =>
  Printf.sprintf(
    "[line: %d, char: %d-%d]",
    start.Lexing.pos_lnum,
    start.Lexing.pos_cnum - start.Lexing.pos_bol,
    end_.Lexing.pos_cnum - end_.Lexing.pos_bol,
  );

let makeError = (~input, ~start: Lexing.position, ~end_: Lexing.position, exn) => {
  let exnToString = Printexc.to_string(exn);
  let pointerRange = String.make(end_.pos_cnum - start.pos_cnum, '^');

  "\n"
  ++ "Input:"
  ++ indent(1)
  ++ Chalk.green(Chalk.bold(input))
  ++ "\n"
  ++ indent(4)
  ++ String.make(start.pos_cnum, ' ')
  ++ Chalk.gray(pointerRange)
  ++ "\n\n"
  ++ "Error:"
  ++ indent(1)
  ++ Chalk.red(Chalk.bold(exnToString))
  ++ "\n"
  ++ indent(4)
  ++ "Problem parsing at position "
  ++ positionToString(start, end_)
  ++ "\n";
};

let menhir = MenhirLib.Convert.Simplified.traditional2revised(Parser.program);

let last_position = ref(Location.none);

exception LexerError(string);

let provider =
    (~debug, buf): (token, Stdlib__lexing.position, Stdlib__lexing.position) => {
  let (start, stop) = Sedlexing.lexing_positions(buf);
  let token =
    switch (tokenize(buf)) {
    | Ok(t) => t
    | Error(e) => raise(LexerError(e))
    };

  last_position :=
    Location.{loc_start: start, loc_end: stop, loc_ghost: false};

  if (debug) {
    print_endline(token |> show_token);
  };

  (token, start, stop);
};

let parse = (input: string, ~debug: bool): result(expression, string) => {
  let buf = Sedlexing.Utf8.from_string(input);
  let lexer = () => provider(~debug, buf);

  try(Ok(menhir(lexer))) {
  | exn =>
    let Location.{loc_start, loc_end, _} = last_position^;
    Error(makeError(~input, ~start=loc_start, ~end_=loc_end, exn));
  };
};
