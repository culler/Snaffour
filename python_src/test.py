from __future__ import print_function
from doctest import testmod
from . import *
results = testmod(F4base)
print('Doctest results:', results)

# Elizabeth Arnold's example.
# This amazing little example is easy over Fp, but very hard over Q, where some of
# the intermediate polynomial coefficients involve integers with 80,000 digits.
# It makes a pretty convincing case that working mod p and lifting is a much
# better idea than working directly over Q.
R3 = PolyRing('x', 'y', 'z')
A = [
R3.Polynomial({(2,2,0):8, (1,3,0):5, (3,0,1):3, (2,1,1):1}),
R3.Polynomial({(5,0,0):1, (0,3,2):2, (0,2,3):13, (0,1,4):5}),
R3.Polynomial({(3,0,0):8, (0,3,0):12, (1,0,2):2, (0,0,0):3}),
R3.Polynomial({(2,4,0):7, (1,3,2):18, (0,3,3):1})
]

# Cyclic-4
R4 = PolyRing('a', 'b', 'c', 'd')
C4 = [
R4.Polynomial({(1,1,1,1): 1, (0,0,0,0): -1}),
R4.Polynomial({(1,1,1,0): 1, (1,1,0,1): 1, (1,0,1,1): 1, (0,1,1,1): 1}),
R4.Polynomial({(1,1,0,0): 1, (0,1,1,0): 1, (0,0,1,1): 1, (1,0,0,1): 1}),
R4.Polynomial({(1,0,0,0): 1, (0,1,0,0): 1, (0,0,1,0): 1, (0,0,0,1): 1}),
]

# Cyclic-5
R5 = PolyRing('a', 'b', 'c', 'd', 'e')
C5 = [
R5.Polynomial({(1,1,1,1,1): 1, (0,0,0,0,0): -1}),
R5.Polynomial({(1,1,1,1,0): 1, (1,1,1,0,1): 1, (1,1,0,1,1): 1, (1,0,1,1,1): 1, (0,1,1,1,1): 1}),
R5.Polynomial({(1,1,1,0,0): 1, (1,1,0,0,1): 1, (1,0,0,1,1): 1, (0,0,1,1,1): 1, (0,1,1,1,0): 1}),
R5.Polynomial({(1,1,0,0,0): 1, (1,0,0,0,1): 1, (0,0,0,1,1): 1, (0,0,1,1,0): 1, (0,1,1,0,0): 1}),
R5.Polynomial({(1,0,0,0,0): 1, (0,0,0,0,1): 1, (0,0,0,1,0): 1, (0,0,1,0,0): 1, (0,1,0,0,0): 1}),
]

# Cyclic-6
R6 = PolyRing('a', 'b', 'c', 'd', 'e', 'f')
C6 = [
R6.Polynomial({(1,1,1,1,1,1): 1, (0,0,0,0,0,0): -1}),
R6.Polynomial({(1,1,1,1,1,0): 1, (1,1,1,1,0,1): 1, (1,1,1,0,1,1): 1, (1,1,0,1,1,1): 1,
               (1,0,1,1,1,1): 1, (0,1,1,1,1,1): 1}),
R6.Polynomial({(1,1,1,1,0,0): 1, (1,1,1,0,0,1): 1, (1,1,0,0,1,1): 1, (1,0,0,1,1,1): 1,
               (0,0,1,1,1,1): 1, (0,1,1,1,1,0): 1}),
R6.Polynomial({(1,1,1,0,0,0): 1, (1,1,0,0,0,1): 1, (1,0,0,0,1,1): 1, (0,0,0,1,1,1): 1,
               (0,0,1,1,1,0): 1, (0,1,1,1,0,0): 1}),
R6.Polynomial({(1,1,0,0,0,0): 1, (1,0,0,0,0,1): 1, (0,0,0,0,1,1): 1, (0,0,0,1,1,0): 1,
               (0,0,1,1,0,0): 1, (0,1,1,0,0,0): 1}),
R6.Polynomial({(1,0,0,0,0,0): 1, (0,0,0,0,0,1): 1, (0,0,0,0,1,0): 1, (0,0,0,1,0,0): 1,
               (0,0,1,0,0,0): 1, (0,1,0,0,0,0): 1})
]

# Cyclic-7
R7 = PolyRing('a', 'b', 'c', 'd', 'e', 'f', 'g')
C7 = [
R7.Polynomial({(1,1,1,1,1,1,1): 1, (0,0,0,0,0,0,0): -1}),
R7.Polynomial({(1,1,1,1,1,1,0): 1, (1,1,1,1,1,0,1): 1, (1,1,1,1,0,1,1): 1, (1,1,1,0,1,1,1): 1,
               (1,1,0,1,1,1,1): 1, (1,0,1,1,1,1,1): 1, (0,1,1,1,1,1,1): 1}),
R7.Polynomial({(1,1,1,1,1,0,0): 1, (1,1,1,1,0,0,1): 1, (1,1,1,0,0,1,1): 1, (1,1,0,0,1,1,1): 1,
               (1,0,0,1,1,1,1): 1, (0,0,1,1,1,1,1): 1, (0,1,1,1,1,1,0): 1 }),
R7.Polynomial({(1,1,1,1,0,0,0): 1, (1,1,1,0,0,0,1): 1, (1,1,0,0,0,1,1): 1, (1,0,0,0,1,1,1): 1,
               (0,0,0,1,1,1,1): 1, (0,0,1,1,1,1,0): 1, (0,1,1,1,1,0,0): 1}),
R7.Polynomial({(1,1,1,0,0,0,0): 1, (1,1,0,0,0,0,1): 1, (1,0,0,0,0,1,1): 1, (0,0,0,0,1,1,1): 1,
               (0,0,0,1,1,1,0): 1, (0,0,1,1,1,0,0): 1, (0,1,1,1,0,0,0): 1}),
R7.Polynomial({(1,1,0,0,0,0,0): 1, (1,0,0,0,0,0,1): 1, (0,0,0,0,0,1,1): 1, (0,0,0,0,1,1,0): 1,
               (0,0,0,1,1,0,0): 1, (0,0,1,1,0,0,0): 1, (0,1,1,0,0,0,0): 1}),
R7.Polynomial({(1,0,0,0,0,0,0): 1, (0,0,0,0,0,0,1): 1, (0,0,0,0,0,1,0): 1, (0,0,0,0,1,0,0): 1,
               (0,0,0,1,0,0,0): 1, (0,0,1,0,0,0,0): 1, (0,1,0,0,0,0,0): 1})
]

print('\nComputing some Groebner bases over Fp, p = 2^31 - 1\n')

print("Elizabeth Arnold's example:")
print(R3.Ideal(A).reduced_groebner_basis())
print()
print('Cyclic-4 with normal selector:')
print(R4.Ideal(C4).reduced_groebner_basis())
print()
print('Cyclic-4 with identity selector:')
I = R4.Ideal(C4)
I.set_select('id')
GI = I.reduced_groebner_basis()
print(GI)
print()
print('Cyclic-4 with Buchberger selector:')
J = R4.Ideal(C4)
J.set_select('buchberger')
GJ = J.reduced_groebner_basis()
print(GJ)
print()
print('The size of the Cyclic-5 basis:')
G = R5.Ideal(C5).reduced_groebner_basis()
print(len(G))
