! IBM_PROLOG_BEGIN_TAG                                                  
! This is an automatically generated prolog.                            
!                                                                       
! $Source: src/misc/fortran/arkdb_async.f $                                    
!                                                                       
! IBM Data Engine for NoSQL - Power Systems Edition User Library Project
!                                                                       
! Contributors Listed Below - COPYRIGHT 2014,2015                       
! [+] International Business Machines Corp.                             
!                                                                       
!                                                                       
! Licensed under the Apache License, Version 2.0 (the "License");       
! you may not use this file except in compliance with the License.      
! You may obtain a copy of the License at                               
!                                                                       
!     http://www.apache.org/licenses/LICENSE-2.0                        
!                                                                       
! Unless required by applicable law or agreed to in writing, software   
! distributed under the License is distributed on an "AS IS" BASIS,     
! WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       
! implied. See the License for the specific language governing          
! permissions and limitations under the License.                        
!                                                                       
! IBM_PROLOG_END_TAG         

! Usage: arkdb_async /dev/sgX:/dev/sgY

      PROGRAM Benchmark
      use iso_c_binding

      type(c_ptr) :: VLUN1
      integer, parameter :: asize=28000
      real, allocatable, dimension(:,:) :: array1
      real, allocatable, dimension(:,:) :: array2
      real, allocatable, dimension(:,:) :: array3
      real, allocatable, dimension(:,:) :: array4
      real(4) :: start, finish
      integer :: i, j,stat
      integer(c_int64_t) :: flen=5,alen
      integer(c_int64_t), target :: done1,done2,done3,done4
      character*100 :: devs
      call GET_COMMAND_ARGUMENT(1,devs, status=stat)

      allocate (array1(asize,asize))
      allocate (array2(asize,asize))
      allocate (array3(asize,asize))
      allocate (array4(asize,asize))

      alen=sizeof(array1)

      devs=TRIM(ADJUSTL(devs))//char(0)

      forall (i=1:asize, j=1:asize) array1(i,j)=i*asize+j
      forall (i=1:asize, j=1:asize) array2(i,j)=i*asize+j
      forall (i=1:asize, j=1:asize) array3(i,j)=i*asize+j
      forall (i=1:asize, j=1:asize) array4(i,j)=i*asize+j

      call open_vlun(devs, VLUN1)

      call  write_vlun(VLUN1,%val(flen),fn1,%val(alen),array1)

      start = secnds(0.0)

      done1=0
      done2=0
      done3=0
      done4=0
      call awrite_vlun(VLUN1,%val(flen),fn1,%val(alen),array1,done1)
      call awrite_vlun(VLUN1,%val(flen),fn2,%val(alen),array2,done2)
      call awrite_vlun(VLUN1,%val(flen),fn3,%val(alen),array3,done3)
      call awrite_vlun(VLUN1,%val(flen),fn4,%val(alen),array4,done4)

      do while (done1==0 .OR. done2==0 .OR. done3==0 .OR. done4==0)
        call SLEEP(1)
      end do

      finish = secnds(start)
      print '("Write Time = ",f6.3," seconds.")',finish

      call  read_vlun(VLUN1,%val(flen),fn1,%val(alen),array1)

      start = secnds(0.0)

      done1=0
      done2=0
      done3=0
      done4=0
      call aread_vlun(VLUN1,%val(flen),fn1,%val(alen),array1,done1)
      call aread_vlun(VLUN1,%val(flen),fn2,%val(alen),array2,done2)
      call aread_vlun(VLUN1,%val(flen),fn3,%val(alen),array3,done3)
      call aread_vlun(VLUN1,%val(flen),fn4,%val(alen),array4,done4)

      do while (done1==0 .OR. done2==0 .OR. done3==0 .OR. done4==0)
        call SLEEP(1)
      end do

      finish = secnds(start)
      print '("Read Time = ",f6.3," seconds.")',finish

      deallocate (array1)
      deallocate (array2)
      deallocate (array3)
      deallocate (array4)
      call close_vlun(VLUN1)

      END

