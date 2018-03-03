# Outline
In programming models such as OpenMP or OpenACC,
programmers can just implement a sequential program
and annotate which loops can be executed in parallel.
To test the correctness of the implementation,
we may execute the loop iterations in random order
since a parallel loop should not have inter-iteration dependencies.
This project contains two macros LOOP_BEGIN and LOOP_END,
which can be used to enclose loops, 
and the iterations of the enclosed loops will be executed in random order.
The loops can also be nested, nested iterations will be executed in random order
while respecting the fork-join dependencies.

