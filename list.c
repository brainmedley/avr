// Compact extensible event-driven state machine framework

#include <stdlib.h>
//#include <stdio.h>

typedef enum { False = 0, True = 1 } boolean;

/////  Linked List  /////
// GOALS: reusable list data structure; hide implementation details (node structure, malloc / free, ...); efficient stack and queue usage; complete abstract API; minimal overhead
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

int main(void) {
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
