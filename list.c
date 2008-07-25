// Compact extensible event-driven state machine framework

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef enum { False = 0, True = 1 } boolean;

/////  Linked List  /////
// GOALS: reusable list data structure; hide implementation details (node structure, malloc / free, ...); efficient stack and queue use-cases; complete abstract API; minimal overhead
// REM: yes, it takes more space than rolling your own, but I'm sick of writing them over and over and over again
// FIXME: rename private stuff to indicate SPI

typedef struct _list_struct list;
typedef struct _node_struct _node;

struct _list_struct {
  _node *_head;
  _node *_tail;

  _node *_previous;
  _node *_current;
  // REM: count?
};

struct _node_struct {
  void *_item;
  _node *_next;
};


list *create_list() {
  list *l = (list *)malloc(sizeof(list));
  l -> _head = 0;
  l -> _tail = 0;
  l -> _previous = 0;
  l -> _current = 0;
  return l;
}

// ENHANCEME: array2list - takes an array of void*s?


boolean push_head(list *l, void *v) {
  _node *n = (_node *)malloc(sizeof(_node));
  if (n == 0) {
    return False;  // BAIL!
  }

  n -> _item = v;
  n -> _next = l -> _head;
  if (l -> _head == 0) {
    l -> _current = n;
    l -> _tail = n;
  }
  l -> _head = n;
  return True;
}


void *pop_head(list *l) {
  void *v = 0;
  _node *h = l -> _head;
  if (h != 0) {
    v = h -> _item;
    if (h == l -> _tail) {
      l -> _current = 0;  // REM: need to update _previous?
      l -> _tail = 0;
    }
    l -> _head = h -> _next;
    if (h == l -> _current) {
      l -> _current = l -> _head;  // REM: need to update _previous?
    }
    free(h);
  }
  return v;
}

boolean push_tail(list *l, void *v) {
  _node *n = (_node *)malloc(sizeof(_node));
  if (n == 0) {
    return False;  // BAIL!
  }

  n -> _item = v;
  if (l -> _tail == 0) {
    l -> _head = n;
    l -> _current = n;  // REM: need to update _previous?
  } else {
    l -> _tail -> _next = n;
  }
  l -> _tail = n;
  return True;
}

// FIXME: need peek_head / peek_tail (or first_item / last_item)

// ENHANCEME: pop_tail  -- could be made more efficient with crazy xor-linked-list thingie, is it worth it though?  would make negative index support easier, still not sure it's worth it...

// TODO: next / seek_index / seek_item
void *next(list *l) {
  if (0 == l -> _current) return 0;

  l -> _previous = l -> _current;
  l -> _current = l -> _current -> _next;

  if (l -> _current != 0) {
    return l -> _current -> _item;
  } else {
    return 0;
  }
}

void *seek_index(list *l, int i) {
  l -> _current = l -> _head;
  for ( ; i > 0 ; i-- ) {  // REM: is better to test every time and bail or spin off the end and rely on next() not caring?
    next(l);
  }
  if (l -> _current != 0) {
   return l -> _current -> _item;
  } else {
    return 0;
  }
}

void *seek_item(list *l, void *v) {
  l -> _current = l -> _head;
  l -> _previous = 0;
  while (l -> _current != 0 && l -> _current -> _item != v) {
    next(l);
  }
  if (l -> _current != 0) {
   return l -> _current -> _item;
  } else {
    return 0;
  }
}
  

_node *_seek_node(list *l, _node *n) {  // OPTIMIZEME: currently unused, delete if not needed
  l -> _current = l -> _head;
  l -> _previous = 0;
  return l -> _current;
}
  

// ENHANCEME: support negative indicies and slicing


void *current_item(list *l) {
  if (l -> _current == 0) {
    return 0;
  }

  return l -> _current -> _item;
}


boolean add_item(list *l, void *v) {
  _node *n = (_node *)malloc(sizeof(_node));
  if (n == 0) {
    return False;  // BAIL!
  }

  n -> _item = v;
  n -> _next = l -> _current -> _next;
  l -> _current -> _next = n;

  return True;
}


void *drop_current(list *l) {
  // This must be the only thing that requires l -> _previous to be reasonable
  if (l -> _current == 0) {
    return 0;
  } else if (l -> _current == l -> _head) {
    return pop_head(l);  // OPTIMIZEME: wonder if I can obviate this case..
  } // REM: hopefully we can make pop_tail interact well when we get to it
  
  _node *preprevious = 0;
  if (l -> _previous == 0) {
    // A node was just dropped, need to reset _previous
    _node *i = l -> _head;
    while (i != 0 && i -> _next != l -> _current) {
      preprevious = l -> _previous;  // REM: could extract this to a 'preprevious_node()' and use it here and drop_tail
      l -> _previous = i;
      i = i -> _next;
    }
  }

  l -> _previous -> _next = l -> _current -> _next;
  void *v = l -> _current -> _item;
  free(l -> _current);
  l -> _current = l -> _previous;
  l -> _previous = preprevious;

  return v;
}


void destroy_list(list *l) {
  while (l -> _head != 0) {
    pop_head(l);  // OPTIMIZEME: don't have to do all the state maintenence work pop_head does here
  }
  free(l);
}

// TODO: map
//////////

/////  AVR Unit  /////
// GOALS: concise, flexible, portable, unit testing framework & tools capable of running tests either on-computer or on-chip; self-hosting as much as it can be

#define AU_RESULT_MESSAGE_LEN 256
#define AU_RESULTS_LIST_LEN 256

typedef enum { AU_PASS, AU_FAIL, AU_ERROR } result_type;  // REM: tryin' to work out if there's a better set of encodings... perhaps pass = 1, fail = 0, errors represented as failures of some "meta test"...

typedef struct _result_struct {
  result_type _type;
  // TODO: ID in here somewhere, probably eventually w/ type bitfield'd in
  char _message[AU_RESULT_MESSAGE_LEN];
} _result;

_result *results_list[AU_RESULTS_LIST_LEN];
int results_end_index = -1;  // HACK ALERT: there must be a better way to work this than having to start from -1 and blindly incrementing


_result *create_result(result_type type, char *message) {
  _result *result = (_result *)malloc(sizeof(_result));
  result -> _type = type;
  strncpy(result -> _message, message, AU_RESULT_MESSAGE_LEN - 1);
  result -> _message[AU_RESULT_MESSAGE_LEN - 1] = '\0';
  return result;
}

void pass(char message[]) {
  results_end_index++;
  // FIXME: work out some way to abort if we've blown the list (return False? (maybe have returning False return from the test function via macro magic)  have runner check before the next test?)
  _result *result = create_result(AU_PASS, message);
  results_list[results_end_index] = result;
}

void fail(char message[]) {
  results_end_index++;
  // FIXME: work out some way to abort if we've blown the list (return False? (maybe have returning False return from the test function via macro magic)  have runner check before the next test?)
  _result *result = create_result(AU_FAIL, message);
  results_list[results_end_index] = result;
}


void print_results() {
  int pass_count = 0, fail_count = 0, error_count = 0;
  for (int i = 0 ; i <= results_end_index ; i++) {  // FIXME: Pass keeps incrementing the end, so this could prance merrily off the array
    char *type_str;
    _result *result = results_list[i];

    switch (result -> _type) {
    case AU_PASS:
      type_str = "Pass";
      pass_count++;
      break;

    case AU_FAIL:
      type_str = "Fail";
      fail_count++;
      break;

    case AU_ERROR:
      type_str = "Error";
      error_count++;
      break;

    default:
      fprintf(stdout, "Error: unknown type '%d' for result '%d': %s\n", result -> _type, i, result -> _message);
      error_count++;
      break;
    }
    fprintf(stdout, "%s: %s\n", type_str, result -> _message);
  }
  fprintf(stdout, "Pass: %d  --  Fail: %d  --  Error: %d  --  Total: %d\n\n", 
          pass_count, fail_count, error_count, pass_count + fail_count + error_count);
}
//////////


/////  AVR Unit Tests  /////
void run_au_tests() {
  fprintf(stdout, "AU Tests:\n");
  results_end_index = -1;
  //setup_au_tests();
  pass("WOOOOOOOT!!");
  fail("failure");
  //cleanup_au_tests();
  print_results();
}
//////////


/////  List Tests  /////
char itemsArray[] = "abcdefghijklmnopqrstuvwxyz";
char *itemPointer(char c) {
  return &itemsArray[c - 'a'];
}


void test_stack() {
  list *l = create_list();
  char *a = itemPointer('a');
  char *b = itemPointer('b');
  char *c = itemPointer('c');

  if (pop_head(l) == 0) pass("pop_head on empty list returned 0");
  else fail("pop_head on empty list did not return 0");

  push_head(l, a);
  if (pop_head(l) == a) pass("pop_head returned what was pushed");
  else fail("pop_head did not return what was pushed");

  if (pop_head(l) == 0) pass("second pop_head returned 0");
  else fail("second pop_head did not return 0");

  push_head(l, a);
  push_head(l, b);
  push_head(l, c);
  if (pop_head(l) == c) pass("pop_head returned what was pushed last");
  else fail("pop_head did not return what was pushed last");
  if (pop_head(l) == b) pass("pop_head returned what was pushed second");
  else fail("pop_head did not return what was pushed second");
  if (pop_head(l) == a) pass("pop_head returned what was pushed first");
  else fail("pop_head did not return what was pushed first");

  if (pop_head(l) == 0) pass("extra pop_head returned 0");
  else fail("extra pop_head did not return 0");
}


void test_queue() {
  list *l = create_list();
  char *a = itemPointer('a');
  char *b = itemPointer('b');
  char *c = itemPointer('c');

  push_tail(l, a);
  if (pop_head(l) == a) pass("pop_head returned what was pushed onto the tail");
  else fail("pop_head did not return what was pushed onto the tail");

  if (pop_head(l) == 0) pass("second pop_head returned 0");
  else fail("second pop_head did not return 0");

  push_tail(l, a);
  push_tail(l, b);
  push_tail(l, c);
  if (pop_head(l) == a) pass("pop_head returned what was pushed first");
  else fail("pop_head did not return what was pushed first");
  if (pop_head(l) == b) pass("pop_head returned what was pushed second");
  else fail("pop_head did not return what was pushed second");
  if (pop_head(l) == c) pass("pop_head returned what was pushed last");
  else fail("pop_head did not return what was pushed last");

  if (pop_head(l) == 0) pass("extra pop_head returned 0");
  else fail("extra pop_head did not return 0");
}


void test_traversal() {
  list *l = create_list();
  char *a = itemPointer('a');
  char *b = itemPointer('b');
  char *c = itemPointer('c');
  char *d = itemPointer('d');
  char *e = itemPointer('e');
  char *f = itemPointer('f');

  push_tail(l, a);
  if (next(l) == a) pass("next returned what was pushed onto the tail");
  else fail("next did not return what was pushed onto the tail");

  if (next(l) == 0) pass("second pop_head returned 0");
  else fail("second pop_head did not return 0");

  for (int i = 0 ; i < 6 ; i++) {
    push_tail(l, &itemsArray[i]);
  }

  if (pop_head(l) == a) pass("pop_head returned what was pushed first");
  else fail("pop_head did not return what was pushed first");
  if (pop_head(l) == b) pass("pop_head returned what was pushed second");
  else fail("pop_head did not return what was pushed second");
  if (pop_head(l) == c) pass("pop_head returned what was pushed last");
  else fail("pop_head did not return what was pushed last");

  if (pop_head(l) == 0) pass("extra pop_head returned 0");
  else fail("extra pop_head did not return 0");
}


void run_list_tests() {
  fprintf(stdout, "List Tests:\n");
  results_end_index = -1;
  //setup_list_tests();
  test_stack();
  test_queue();
  //cleanup_list_tests();
  print_results();
}
//////////


int main(void) {
  run_au_tests();
  run_list_tests();
  return 0;
}

/*  NOTES:


ianaos: (possibly optional) watchdog callback called if "os size" (the number of tasks + state machine size) gets too large (thus threatening the stack)
ianaos: (possibly optional) watchdog callback called if task queue is detected to be falling behind
ianaos: optional tag byte for debug output by watchdogs & others
ianaos: do math for clock rate and desired tick time
ianaos: possible to do a reasonably accurate "total time" that accounts for the true tick time, rather than the reqested tick time?
ianaos: "external clock" hook, plus default "clock sources" for some common chips  (can / should I choose default speeds / tick times for common chips based on which chip this is being compiled for?  perhaps, maybe #warning though)

ianaos: debug: output helpers - 2-wire, serial, blink single LED in some reasonably readable way (morse code? #define CW_WPM? maybe pulse in such a way that it could be LED or speaker...)
ianaos: debug: configuration for clocking from a button on a counter pin, possibly #if in a bunch of chatter to the debug port in that mode, too

ianaos: optimization: swapable data structures, ringbuffers instead of dynamic linked list, etc (might need a bunch o' #defs to cons "generic api" calls into "specific api" calls and re-define the "list" typedef...  it could be kinda nice to be able to swap on a per-list basis though...

ianaos: question: how will this interact with usb stacks?
ianaos: question: mutex primative?  is there one already?  do we need one?
ianaos: question: am I reinventing wheels I shouldn't?
ianaos: question: feasibility of running unit testing suite on device(s)?  perhaps write test results to eeprom and read them back w/ programmer, prolly still need blinkenlights for progress/status...
ianaos: question: what happens if you write to somewhere you shouldn't? (i.e. under what conditions would a unit test on the device error instead of fail? perhaps the framework should have a watchdog timer?)
ianaow: question: malloc "page" size?  what can be done to use malloc'd memory most efficiently?

ianaos: why: lightweight, transparent, unit tested, portable, application infrastructure and debugging framework

ianaos: goal: blinky light + faster / slower buttons < 1K  (find out how big the naieve implemenation is pre-ianaos)
ianaos: goal: quantified overhead
ianaos: goal: unit tested

ianaos: is not: a utility library, a "beginner platform", ...

ianaos: design: simple scheduler + event queue + state machine = powerful lightweight app framework

Roadmap:
0.0.1: list datastructure + unit tests
0.1: task queue
0.2: add event queue
0.3: add state machine
0.4: cleanup & example apps, perhaps publish (earliest publishable version)
0.5: better timing; better interaction in larger project (BBQ controller)
debugging / watchdogs
optimizations
 */
