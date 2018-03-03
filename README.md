# Outline
In programming models such as OpenMP or OpenACC,
   programmers can just implement a sequential program
   and annotate which loops can be executed in parallel.
To test the correctness of the implementation,
   we may execute the loop iterations in random order
   since a parallel loop should not have inter-iteration dependencies.


