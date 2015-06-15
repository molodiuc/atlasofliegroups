% Copyright (C) 2006-2015 Marc van Leeuwen
% This file is part of the Atlas of Lie Groups and Representations (the Atlas)

% This program is made available under the terms stated in the GNU
% General Public License (GPL), see http://www.gnu.org/licences/licence.html

% The Atlas is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2 of the License, or
% (at your option) any later version.

% The Atlas is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
% GNU General Public License for more details.

% You should have received a copy of the GNU General Public License
% along with the Atlas; if not, write to the Free Software
% Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


\def\point{\item{$\bullet$}}

@* Introduction to the {\tt realex} interpreter.
%
While the Atlas software initially consisted of a single executable
program \.{atlas} that essentially allows running each of the functionalities
provided by the software library separately, a second executable program
called \.{realex} has been developed since the year~$2006$, which program also
gives access to the functionalities provided by the software library, but in a
manner that provides the user with means to capture the results from previous
computations and use them in subsequent computations. It started out as an
interface giving the user just assignable variables and an expression language
for calling library functions, but has grown out to a complete programming
language, which at the time of writing is still being extended in many
directions. The name of the program might stand for Redesigned
Expression-based Atlas of Lie groups EXecutable, until a better explanation
for the name comes along.

The sources specific to the interpreter are situated in the
current (\.{sources/interpreter/}) subdirectory, and almost all its names are
placed in the |atlas::interpreter| namespace. This part of the program has
been split into several modules, only some of which are centred around
specific data structures; in general these modules are more interdependent
than those of the main Atlas library, and the subdivision is more arbitrary
and subject to change. We list here the source files for these modules, and
their dependencies notably on the level of their header files.

\point The file \.{buffer.w} defines the classes |BufferedInput| providing an
interface to input streams, and |Hash_table| storing and providing a
translation from identifiers to small integers |id_type|.

\point The file \.{parsetree.w} defines the |expr| structure representing parsed
expressions, and many related types, and node constructing functions for use
by the parser. Its header file includes \.{buffer.h}, and declares some
pointer types to types that will be defined in \.{types.h}.

\point The file \.{parser.y} is the source for \.{bison}-generated the parser
file \.{parser.tab.c}. The header file \.{parser.tab.h} includes nothing, but
contains definitions depending on \.{parse\_types.h} having been included.

\point The file \.{lexer.w} defines the lexical analyser class
|Lexical_analyser| and the readline completion function |id_completion_func|.
Its header file includes \.{buffer.h}, \.{parse\_types.h} and \.{parser.tab.h}.

\point The file \.{types.w} defines the main base classes for the evaluator,
|type_expr| for representing (realex) types, |value_base| for dynamic values,
|context| for dynamic evaluation contexts, |expression_base| for ``compiled''
expressions, |program_error| for exceptions, and numerous types related to
these. Its header file includes \.{parsetree.h} (which is needed for the error
classes only).

\point The file \.{built-in-types.w} defines types primitive to realex which
encapsulate types of the Atlas library, and their interface functions. Its
header file includes \.{types.h} and many headers from the Atlas library.

\point The file \.{global.w} defines other primitive types less intimately
related to Atlas, like integers, rationals, matrices. Also some global aspects
of the interpreter like operating the global identifier tables. Its header
file includes \.{types.h} and some headers from the Atlas library.

\point The file \.{evaluator.w} Defines the realex type-checker and the
evaluator proper. It defines many classes derived from |expression_base|. Its
header file includes \.{lexer.h} and \.{global.h}.

\point The file \.{main.w}, the current module, brings everything together and
defining the main program. It has no header file.


@* Main program. This file defines a small main program to test the parser
under development. It is written in \Cpp, but is it mainly concerned with
interfacing to the parser that is generated by~\.{bison}, and which is
therefore written in~\Cee. However, we now compile the generated parser file
\.{parser.tab.c} using a \Cpp\ compiler, which means we can write our code
here as an ordinary \Cpp\ program, including use of namespaces.

Since depending on the readline libraries still gives difficulties on some
platforms, we arrange for the possibility of compiling this program in the
absence of that library. This means that we should refrain from any reference
to its header files, and so the corresponding \&{\#include} statements cannot
be given in the usual way, which would cause their inclusion unconditionally.
Like for the \.{atlas} program, the compile time flag |NREADLINE|, if defined
by setting \.{-DNREADLINE} as a flag to the compiler, will prevent any
dependency on the readline library.

@d realex_version "0.9"
 // numbering from 0.5 (on 27/11/2010); last change May 6, 2015

@c

@< Conditionally include the header files for the readline library @>

@< Declaration of interface to the parser @>@;
namespace { @< Local static data @>@; }@;
@< Definitions of global namespace functions @>@;
namespace atlas { namespace interpreter {@< Definitions of other functions @>@;
}@;}@;
@< Main program @>

@ Here are some header files which need to be included for this main program.
As we discussed above, the inclusion of header files for the readline
libraries is made dependent on the flag |NREADLINE|. In case the flag is set,
we define the few symbols used from the readline library as macros, so that
the code using them can be compiled without needing additional \&{\#ifdef}
lines. It turns out that |getline| and |add_history| are not used in calls,
but rather passed as function pointers to the |BufferedInput| constructor, so
the appropriate expansion for these macros is the null pointer.

@h <iostream>
@h <fstream>

@h "buffer.h"
@h "lexer.h"
@h "version.h"

@< Conditionally include the header files for the readline library @>=
#ifdef NREADLINE
#define readline nullptr
#define add_history nullptr
#define clear_history()
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

@ Since the file \.{parser.y} declares \.{\%pure-parser} and \.{\%locations},
the prototype of the lexical analyser (wrapper) function |yylex| is the one
below. Curiously, the program~\.{bison} does not write this prototype to
\.{parser.tab.h}, but it does write the definitions of the types |YYSTYPE| and
|YYLTYPE| there; these require that \.{parse\_types.h} be included first. We
also declare ``{\tt\%parse-param \char`\{} |int* verbosity, expr_p*
parsed_expr@;| {\tt\char`\}}'' in~\.{parser.y}, so that the parser itself,
|yyparse|, takes an integer pointer as parameter, which it uses to signal
special requests from the user (such as verbose output but also termination or
output redirection), and a pointer to an expression, in which it writes the
result of parsing.

The definitions below used to start with |extern "C"|, but no longer do so
since the parser is now compiled as a \Cpp\ program.

@h "parse_types.h"
@h "parser.tab.h"

@< Declaration of interface to the parser @>=

int yylex (YYSTYPE *, YYLTYPE *);
@/int yyparse( atlas::interpreter::expr_p* parsed_expr, int* verbosity );

@ Here are the wrapper function for the lexical analyser and the error
reporting function, which are necessary because the parser cannot directly
call a class method. The prototypes are imposed, in particular the second and
third arguments to |yyerror| are those passed to |yyparse|, even though they
are not used in |yyerror|. In |yyerror| we close any open include files, as
continuing to execute their commands is undesirable.

@< Definitions of global namespace functions @>=

int yylex(YYSTYPE *valp, YYLTYPE *locp)
{@; return atlas::interpreter::lex->get_token(valp,locp); }
@)

void yyerror (YYLTYPE* locp, atlas::interpreter::expr_p* ,int* ,char const *s)
{ atlas::interpreter::main_input_buffer->show_range@|
  (std::cerr,
   locp->first_line, locp->first_column,
   locp->last_line,  locp->last_column);
  std::cerr << s << std::endl;
  atlas::interpreter::main_input_buffer->close_includes();
}

@ We have a user interrupt handler that simple raises |interrupt_flag| and
returns. It must be declares |extern "C"|.

@< Definitions of global namespace functions @>=

extern "C" void sigint_handler (int)
@+{@; atlas::interpreter::interrupt_flag=1; }

@ The function |id_completion_func| defined in the \.{lexer} module will not
be plugged directly into the readline completion mechanism, but instead we
provide an alternative function for generating matches, which may pass the
above function to |rl_completion_matches| when it deems the situation
appropriate, or else returns |nullptr| to indicate that the default function,
completing on file names, should be used instead.

@< Definitions of global namespace functions @>=
#ifndef NREADLINE
extern "C" char** do_completion(const char* text, int start, int end)
{
  if (start>0)
  { int i; char c; bool need_file=false;
    for (i=0; i<start; ++i)
      if (std::isspace(c=rl_line_buffer[i]))
        continue; // ignore space characters
      else if (c=='<' or c=='>')
        need_file=true;
      else
        break;

    if (need_file and i==start)
       // the text is preceded by one or more copies of \.<, \.>
      return nullptr; // signal that file name completion should be used
  }
  rl_attempted_completion_over = true;
    // don't try file name completion if we get here
  return rl_completion_matches(text,atlas::interpreter::id_completion_func);
}
#endif

@ The code concerning the \.{readline} library is excluded in cas
the \.{NREADLINE} flag is set.

@< Initialise the \.{readline} library interface @>=
#ifndef NREADLINE
  using_history();
  rl_completer_word_break_characters = lexical_break_chars;
  rl_attempted_completion_function = do_completion; // set up input completion

#endif

@ Here is an array that declares the keywords that the lexical scanner is to
recognise, terminated by a null pointer. Currently the lexical analyser adds
the offset of the keyword in this list to |QUIT|, so the recognition depends
on the fact that |"quit"| is the first keyword, and that they are listed below
in the same order as in the \.{\%token} declarations in \.{parser.y}.

@< Local static data @>=

const char* keywords[] =
 {"quit"
 ,"set","let","in","begin","end"
 ,"if","then","else","elif","fi"
 ,"and","or","not"
 ,"while","do","od","next","for","from","downto"
 ,"case","esac", "rec_fun"
 ,"true","false", "die"
 ,"whattype","showall","forget"
 ,nullptr};

@ After installing keywords in the lexical analyser, some more preparation is
needed. The identifiers |quiet| and |verbose| that used to be keywords are now
instead recognised only in the special commands \.{set quiet} and \.{set
verbose}; to this end the parser uses their numeric identifier codes. To
ensure that they are respectively at offsets $0,1$ of
|ana.first_identifier()|, we look up these names before any other identifiers
are introduced, notably before |initialise_evaluator| and
|initialise_builtin_types| are called to define built-in operators and
functions.

@< Prepare the lexical analyser... @>=
main_hash_table->match_literal("quiet");
main_hash_table->match_literal("verbose");
// these must be the very first identifiers
ana.set_comment_delims('{','}');

@ Here are several calls necessary to get various parts of this program off to
a good start, starting with the history and readline libraries, and setting a
comment convention. Initialising the constants in the Atlas library is no
longer necessary, as it is done automatically before |main| is called. Our own
compilation units do require explicit calling of their initialisation
functions.

@h <csignal>
@h "built-in-types.h"

@< Initialise various parts of the program @>=
  @< Initialise the \.{readline} library interface @>
  signal(SIGINT,sigint_handler); // install handler for user interrupt
  initialise_evaluator();
  initialise_builtin_types();
  { id_type shriek = main_hash_table->match_literal("!");
    const auto& variants = global_overload_table->variants(shriek);
    for (auto it=variants.begin(); it!=variants.end(); ++it)
      if (it->type().arg_type==bool_type)
      { boolean_negate_builtin =
          std::dynamic_pointer_cast<const builtin_value>(it->val);
         break;
      }
    assert(boolean_negate_builtin.get()!=nullptr);
  }


@ Our main program constructs unique instances
for various classes of the interpreter, and sets pointers to them so that
various compilation units can access them. Then it processes command line
arguments and does some more declarations that can depend on them.
Finally it executes the main command
loop, from which normally only the \.{quit} command will make it exit.

@h <unistd.h> // for |isatty|
@< Main program @>=

int main(int argc, char** argv)
{ using namespace atlas::interpreter;
@)

@/Hash_table hash; main_hash_table= &hash;
@/Id_table main_table; @+ global_id_table=&main_table;
@/overload_table main_overload_table;
 @+ global_overload_table=&main_overload_table;
@)
  bool use_readline=true;
  @< Other local variables of |main| @>
  @< Handle command line arguments @>
@/BufferedInput input_buffer(isatty(STDIN_FILENO) ? "expr> " : nullptr
                            ,use_readline ? readline : nullptr
			    ,use_readline ? add_history : nullptr);
  main_input_buffer= &input_buffer;
@/Lexical_analyser ana(input_buffer,hash,keywords,prim_names); lex=&ana;
  @< Prepare the lexical analyser |ana| after construction and before use @>
@/@< Initialise various parts of the program @>
  @< Enter system variables into |global_id_table| @>
@)
  @< Silently read in the files from |prelude_filenames| @>
  std::cout << "This is 'realex', version " realex_version " (compiled on " @|
            << atlas::version::COMPILEDATE @| <<
").\nIt is the programmable interpreter interface to the library (version " @|
       << atlas::version::VERSION @| << ") of\n"
       << atlas::version::NAME << @| ". http://www.liegroups.org/\n";
@)
  @< Enter the main command loop @>
  signal(SIGINT,SIG_DFL); // reinstall default signal handler
  clear_history();
  // clean up (presumably disposes of the lines stored in history)
  std::cout << "Bye.\n";
  return 0;
}

@ When reading command line arguments, some options may specify a search path,
any non-option arguments will considered to be file names (forming the
``prelude''). We shall gather both in lists of strings.

@< Other local variables of |main| @>=
std::vector<const char*> paths,prelude_filenames;
paths.reserve(argc-1); prelude_filenames.reserve(argc-1);

@ The strings in |paths| will initialise a ``system variable'' created below
(a variable the user can assign to, and which is inspected whenever files are
opened). We shall use a static variable that gives access to its value. It
continues to do so even if the user should manage to forget or hide the user
variable introduced below to hold it.

@h "global.h" // defines |shared_share|
@< Local static data @>=
static atlas::interpreter::shared_share input_path_pointer,prelude_log_pointer;

@ Apart from the \.{--no-readline} option to switch off the readline functions
(which might be useful when input comes from a file), the program accepts
options that set the search path for scripts, and a number of scripts that
form the ``prelude''. The readline option must be read early to influence the
constructor of the lexical analyser, but the other options are just stored
away here for later processing.

@h <cstring>

@< Handle command line arguments @>=
while (*++argv!=nullptr)
{ static const char* const path_opt = "--path=";
  static const size_t pol = std::strlen(path_opt);
  std::string arg(*argv);
  if (arg=="--no-readline")
    {@; use_readline = false; continue; }
  if (arg.substr(0,pol)==path_opt)
     paths.push_back(&(*argv)[pol]);
  else prelude_filenames.push_back(*argv);
}

@ Here we create the system variables called |input_path| and |prelude_log|;
both are lists of strings, the latter a constant one.

@< Enter system variables into |global_id_table| @>=
{
  own_value input_path = std::make_shared<row_value>(paths.size());
  id_type ip_id = main_hash_table->match_literal("input_path");
  auto oit = force<row_value>(input_path.get())->val.begin();
  for (auto it=paths.begin(); it!=paths.end(); ++it)
    *oit++ = std::make_shared<string_value>(std::string(*it)+'/');
@/global_id_table->add@|(ip_id
                       ,input_path, mk_type_expr("[string]"), false);
  input_path_pointer = global_id_table->address_of(ip_id);
@)
  own_value prelude_log = std::make_shared<row_value>(0); // start out empty
  id_type pl_id = main_hash_table->match_literal("prelude_log");
  auto& logs = force<row_value>(input_path.get())->val;
  logs.reserve(prelude_filenames.size());
@/global_id_table->add@|(pl_id
                       ,prelude_log, mk_type_expr("[string]"), true);
  prelude_log_pointer = global_id_table->address_of(pl_id);
}

@ We can now define the functions that are used in \.{buffer.w} to access the
input path.

@< Definitions of other functions @>=

unsigned int input_path_size()
{ const row_value* path = force<row_value>(input_path_pointer->get());
@/return path->val.size();
}
const std::string& input_path_component(unsigned int i)
{ const row_value* path = force<row_value>(input_path_pointer->get());
  const string_value* dir = force<string_value>(path->val[i].get());
@/return dir->val;
}

@ The command loop maintains two global variables that were defined
in \.{evaluator.w}, namely |last_type| and |last_value|; these start off in a
neutral state. In a loop we call the parser until it sets |verbosity<0|, which
is done upon seeing the \.{quit} command. We call the |reset| method of the
lexical scanner before calling the parser, which will discard any input that
is left by a possible previous erroneous input. This also already fetches a
new line of input, or abandons the program in case none can be obtained.

@< Enter the main command loop @>=
last_value = shared_value (new tuple_value(0));
last_type = void_type.copy();
 // |last_type| is a |type_ptr| defined in \.{evaluator.w}
while (ana.reset()) // get a fresh line for lexical analyser, or quit
{ expr_p parse_tree;
  int old_verbosity=verbosity;
  std::ofstream redirect; // if opened, this will be closed at end of loop
  if (yyparse(&parse_tree,&verbosity)!=0)
     // syntax error (inputs are closed) or non-expression
    continue;
  if (verbosity!=0) // then some special action was requested
  { if (verbosity<0)
      break; // \.{quit} command
    if (verbosity==2 or verbosity==3)
      // indicates output redirection was requested
    { @< Open |redirect| to specified file, and if successful make
      |output_stream| point to it; otherwise |continue| @>
      verbosity=old_verbosity; // verbosity change was temporary
    }
    if (verbosity==1) //
      std::cout << "Expression before type analysis: " << *parse_tree
                << std::endl;
  }
  interrupt_flag=0; // clear interrupt before starting evaluation
  @< Analyse types and then evaluate and print, or catch runtime or other
     errors @>
  output_stream= &std::cout; // reset output stream if it was changed
}

@ If a type error is detected by |analyse_types|, then it will have signalled
it and thrown a |runtime_error|; if that happens |type_OK| will remain |false|
and the runtime error is silently caught. If the result is an empty tuple, we
suppress printing of the uninteresting value.

@h <stdexcept>
@h "evaluator.h"

@< Analyse types and then evaluate and print... @>=
{ bool type_OK=false;
  try
  { expression_ptr e;
    type_expr found_type=analyse_types(*parse_tree,e);
    type_OK=true;
    if (verbosity>0)
      std::cout << "Type found: " << found_type << std::endl @|
	        << "Converted expression: " << *e << std::endl;
    e->evaluate(expression_base::single_value);
@)  // now that evaluation did not |throw|, we can record the predicted type
    last_type = std::move(found_type);
    last_value=pop_value();
    if (last_type!=void_type)
      *output_stream << "Value: " << *last_value << std::endl;
    destroy_expr(parse_tree);
  }
  @< Various |catch| phrases for the main loop @>
}

@ Before entering that main loop, we do a simplified version of the command
loop to read in the prelude files. We do not accept anything that changes
|verbosity| (like trying to call \.{quit}) during the prelude, do not maintain
a last value, and in case of type or runtime errors complain more succinctly
than in the main loop. All non-error output goes to a component of the
|prelude_log| user variable. This is mainly due to the assignment
|output_stream = &log_stream;| and the fact that functions in \.{global.w} use
this pointer; the \.{set} commands that form the essence of prelude files
return a nonzero value from |yyparse|, so in practice most of the code below
is rarely executed.

Errors will break from the inner loop simply by popping the open include
file(s) from |main_input_buffer|. We do not attempt to break from the outer
loop upon an error, as this circumstance is rather hard to detect: any error
has already been reported on |std::cerr|, and leaves a situation not very
different from successfully completing reading the file.

@< Silently read in the files from |prelude_filenames| @>=
for (auto it=prelude_filenames.begin(); it!=prelude_filenames.end(); ++it )
{ std::ostringstream log_stream; output_stream = &log_stream;
  main_input_buffer->push_file(*it,true);
    // set up to read |fname|, unless already done
  while (main_input_buffer->include_depth()>0) // go on until file ends
  { if (not ana.reset())
    { std::cerr << "Internal error, getline fails reading " << *it
                  << std::endl;
      return EXIT_FAILURE;
    }
    expr_p parse_tree;
    if (yyparse(&parse_tree,&verbosity)!=0)
      continue; // if a syntax error was signalled input has been closed
    if (verbosity!=0)
    { std::cerr << "Cannot "
                << (verbosity<0 ? "quit" :
                    verbosity==1 ? "set verbose" : "redirect output")
                         << " during prelude.\n";
      verbosity=0; main_input_buffer->close_includes();
    }
    else
    { try
      { expression_ptr e; type_expr found_type=analyse_types(*parse_tree,e);
        e->evaluate(expression_base::single_value);
        if (found_type!=void_type)
          log_stream << "Value: " << pop_value();
        else
          pop_value(); // don't forget to cast away that void value
      }
      catch (std::exception& err)
      { std::cerr << err.what() << std::endl;
        reset_evaluator(); main_input_buffer->close_includes();
      }
    }
    destroy_expr(parse_tree);
  }
  value pl_val = uniquify(*prelude_log_pointer);
  row_value* logs = force<row_value>(pl_val);
  logs->val.emplace_back(std::make_shared<string_value>(log_stream.str()));
  output_stream = &std::cout;
}

@ The |std::ofstream| object was already created earlier in the main loop,
but it will only be opened if we come here. If this fails then we report it
directly and |continue| to the next iteration of the main loop, which is more
practical at this point than throwing and catching an error.

@< Open |redirect| to specified file... @>=
{ redirect.open(ana.scanned_file_name() ,std::ios_base::out |
     (verbosity==2 ? std::ios_base::trunc : std::ios_base::@;app));
  if (redirect.is_open())
    output_stream = &redirect;
  else
  {@; std::cerr << "Failed to open " << ana.scanned_file_name() << std::endl;
    continue;
  }
}

@ We distinguish runtime errors (which are normal) from internal errors (which
should not happen), and also |catch| and report any other error derived from
|error_base| that could be thrown. After any of these errors we close all
open auxiliary input files; reporting where we were reading is done by the
method |close_includes| defined in \.{buffer.w}.

@< Various |catch| phrases for the main loop @>=
catch (const runtime_error& err)
{ if (type_OK)
    std::cerr << "Runtime error:\n  " << err.what() << "\nEvaluation aborted.";
  else std::cerr << err.what();
  std::cerr << std::endl;
  reset_evaluator(); main_input_buffer->close_includes();
}
catch (const logic_error& err)
{ std::cerr << "Internal error: " << err.what() << "\nEvaluation aborted.\n";
  reset_evaluator(); main_input_buffer->close_includes();
}
catch (const std::exception& err)
{ std::cerr << err.what() << "\nEvaluation aborted.\n";
  reset_evaluator(); main_input_buffer->close_includes();
}

@* Index.

% Local IspellDict: british
