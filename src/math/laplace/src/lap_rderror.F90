!==============================================================================!
subroutine lap_rderror(errbnd,rnge,nlap)
!------------------------------------------------------------------------------!
!
! Read laplace data file of pre-tabulated Laplace exponents and weights
!  of Takatsuka, Ten-no, and Hackbusch.
! Those initial values depend on the interval.
!
!------------------------------------------------------------------------------!

 implicit none

#include "consts.h"
#cmakedefine LAPLACE_ROOT "@LAPLACE_ROOT@"
#cmakedefine LAPLACE_INSTALL "@LAPLACE_INSTALL@"

! constants:
 logical, parameter :: locdbg = .false.
 character(len=*), parameter :: chrdbg = 'lap_rderror>'

 integer, parameter :: luinit = 11

! dimensions:
 integer, intent(in)    :: nlap

! input:
 real(8), intent(in)    :: rnge(2)

! in/output:
 real(8), intent(inout) :: errbnd(2)

! local:
 integer :: iopt, ios, itmp1, itmp2
 logical :: found_token, lower, file_exists
 character(len=5)  :: ctmp
 character(len=11) :: token, cdummy
 character(len=80) :: line
 character(len=512) :: rootdir, filerr
 real(8) :: rlen, dtmp
 
 if (locdbg) write(istdout,"(a)") chrdbg//"enter lap_rderror ..."

 ! get absolute path to data files
 !call getenv("LAPLACE_ROOT",rootdir)
 rootdir = adjustl(LAPLACE_ROOT)
 filerr  = trim(rootdir)//"/data/init_error.txt"
 
 inquire(file=filerr, exist=file_exists)
 
 IF (.NOT. file_exists) THEN
   rootdir = adjustl(LAPLACE_INSTALL)
   filerr  = trim(rootdir)//"/data/init_error.txt"
 ENDIF

 rlen = rnge(2)/rnge(1)

 ! prepare token for reading
 token(1:4) = '1_xk'
 write(token(5:6),'(I2.2)') nlap
 
 open(unit=luinit,file=filerr,status='old',action='read')

 found_token = .false.

 lower = .false.

 do while (.not.found_token)

  ! check for tokens with number of Laplace points
  read(luinit,'(a)',iostat=ios) line
  if (ios.eq.-1) exit
  iopt = index(line,token(1:6))

  ! find tokens with upper bound
  if (iopt.gt.0) then
   line = adjustl(line)
   line = trim(line)

   read(line(8:8),"(I1)") itmp1
   if (line(9:9).eq."E") then
    read(line(10:11),"(I2)") itmp2
    dtmp = real(itmp1,8)*10.d0**itmp2
   else
    read(line(9:11),"(I3)") itmp2
    dtmp = real(itmp1,8)+real(itmp2,8)/1000.d0
   end if

   if (dtmp.le.rlen) then
    ctmp(1:5) = line(7:11)
    lower = .true.
   else
    if (.not.lower) then
     token(7:11) = line(7:11)
    else
     token(7:11) = ctmp(1:5)
    end if
    found_token = .true.
   end if
  end if

 end do

 if (locdbg) write(istdout,*) chrdbg, "token:", token(1:11)

 if (.not.found_token) then
  write(istdout,'(/a,/,a,e10.3,a,/,a,e10.3,a/)') &
    '          W A R N I N G    ','Range of denominator 1.0 -- ',rlen,&
    ' not in the range of pre-tabulated values!',' Take ',dtmp,'instead!'
  token(7:11) = ctmp(1:5)
 end if

 rewind(luinit)

 do
  ! check now for the full token
  read(luinit,'(a)',iostat=ios) line
  if (ios.eq.-1) exit
  iopt = index(line,token(1:10))

  if (iopt.gt.0) then
   read(line,"(a10,1x,ES10.3)") cdummy, errbnd(1)
   errbnd(2) = d0
   exit
  end if
 end do

 close(unit=luinit)
 
end subroutine lap_rderror
!==============================================================================!
