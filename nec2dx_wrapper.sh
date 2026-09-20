#!/bin/bash
# Wrapper for nec2dx that initializes required scratch files

# Create empty unformatted scratch files that nec2dx expects
# Unit 21 is used for antenna radiation pattern caching

# Create a Fortran program to initialize scratch files
cat > /tmp/init_scratch_files.f << 'EOF'
      PROGRAM INIT_SCRATCH
      IMPLICIT NONE
      INTEGER NXA(2)
      REAL*8 AR1, AR2, AR3, EPSCF, DXA, DYA, XSA, YSA
      INTEGER NYA
      
C     Initialize and write dummy data to unit 21
      OPEN(UNIT=21, STATUS='SCRATCH', ACCESS='UNFORMATTED',
     &     ACTION='READWRITE')
      
C     Write a dummy record to initialize the file
      NXA(1) = 0
      NXA(2) = 0
      AR1 = 0.0D0
      AR2 = 0.0D0
      AR3 = 0.0D0
      EPSCF = 0.0D0
      DXA = 0.0D0
      DYA = 0.0D0
      XSA = 0.0D0
      YSA = 0.0D0
      NYA = 0
      
      WRITE(21) AR1, AR2, AR3, EPSCF, DXA, DYA, XSA, YSA, NXA, NYA
      REWIND(21)
      
      CLOSE(21)
      END PROGRAM INIT_SCRATCH
EOF

# Compile and run the initializer
gfortran /tmp/init_scratch_files.f -o /tmp/init_scratch_files 2>/dev/null
if [ -x /tmp/init_scratch_files ]; then
  /tmp/init_scratch_files 2>/dev/null
fi

# Run nec2dx with the provided arguments
exec "$(dirname "$0")/nec2dx.orig" "$@"
