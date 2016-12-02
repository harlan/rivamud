# rivamud
A text-based mud written in only basic C

Dec 2 2016

# BACKGROUND

In the 1990s, Rob, Harlan and Riva made a game called "rivamud".   A small
text-based mod (ala ravenmud) where you could walk through the house and fight.

It was the first time we:
* Wrote a larger C project with meaningful memory management.
* Wrote a tcp/ip daemon networked app.
* Wrote a threaded app and deal with all the locking and threadsafety issues.

While the project was maintained in a cvs repository, the computer it was
maintained on (sdf1.cc) was lost.


# THE GOAL

* Reproduce rivamud.
* Stick to fundamentals:  make + C with standard unix posix libraries 
  (pthreads, tcpip).
* Don't link in mysql, python or a bunch of cute opensource libraries to 
  make it easy.
