subroutine amrex_probinit (init,name,namlen,problo,probhi) bind(c)

  use eos_module
  use eos_type_module
  use amrex_error_module 
  use network
  use probdata_module

  use amrex_fort_module, only : rt => amrex_real
  implicit none

  integer init, namlen
  integer name(namlen)
  real(rt)         problo(3), probhi(3)
  real(rt)         xn(nspec)

  integer untin,i

  type (eos_t) :: eos_state

  namelist /fortin/ p_l, u_l, rho_l, p_r, u_r, rho_r, T_l, T_r, frac, idir, &
       use_Tinit, &
       xcloud, cldradius

  !
  !     Build "probin" filename -- the name of file containing fortin namelist.
  !     
  integer maxlen
  parameter (maxlen=256)
  character probin*(maxlen)

  if (namlen .gt. maxlen) then
     call amrex_error("probin file name too long")
  end if

  do i = 1, namlen
     probin(i:i) = char(name(i))
  end do

  ! set namelist defaults

  p_l = 1.0               ! left pressure (erg/cc)
  u_l = 0.0               ! left velocity (cm/s)
  rho_l = 1.0             ! left density (g/cc)
  T_l = 1.0

  p_r = 0.1               ! right pressure (erg/cc)
  u_r = 0.0               ! right velocity (cm/s)
  rho_r = 0.125           ! right density (g/cc)
  T_r = 1.0

  idir = 1                ! direction across which to jump
  frac = 0.5              ! fraction of the domain for the interface

  use_Tinit = .false.     ! optionally use T_l/r instead of p_l/r for initialization

  xcloud = -1.0
  cldradius = -1.0

  !     Read namelists
  untin = 9
  open(untin,file=probin(1:namlen),form='formatted',status='old')
  read(untin,fortin)
  close(unit=untin)

  split(1) = frac*(problo(1)+probhi(1))
  split(2) = frac*(problo(2)+probhi(2))
  split(3) = frac*(problo(3)+probhi(3))
  
  ! compute the internal energy (erg/cc) for the left and right state
  xn(:) = 0.0e0_rt
  xn(1) = 1.0e0_rt

  if (use_Tinit) then

     eos_state%rho = rho_l
     eos_state%T = T_l
     eos_state%xn(:) = xn(:)

     call eos(eos_input_rt, eos_state)
 
     rhoe_l = rho_l*eos_state%e
     p_l = eos_state%p

     eos_state%rho = rho_r
     eos_state%T = T_r
     eos_state%xn(:) = xn(:)

     call eos(eos_input_rt, eos_state)
 
     rhoe_r = rho_r*eos_state%e
     p_r = eos_state%p

  else

     eos_state%rho = rho_l
     eos_state%p = p_l
     eos_state%T = 100000.e0_rt  ! initial guess
     eos_state%xn(:) = xn(:)

     call eos(eos_input_rp, eos_state)
 
     rhoe_l = rho_l*eos_state%e
     T_l = eos_state%T

     eos_state%rho = rho_r
     eos_state%p = p_r
     eos_state%T = 100000.e0_rt  ! initial guess
     eos_state%xn(:) = xn(:)

     call eos(eos_input_rp, eos_state)
 
     rhoe_r = rho_r*eos_state%e
     T_r = eos_state%T

  endif

end subroutine amrex_probinit


! ::: -----------------------------------------------------------
! ::: This routine is called at problem setup time and is used
! ::: to initialize data on each grid.  
! ::: 
! ::: NOTE:  all arrays have one cell of ghost zones surrounding
! :::        the grid interior.  Values in these cells need not
! :::        be set here.
! ::: 
! ::: INPUTS/OUTPUTS:
! ::: 
! ::: level     => amr level of grid
! ::: time      => time at which to init data             
! ::: lo,hi     => index limits of grid interior (cell centered)
! ::: nstate    => number of state components.  You should know
! :::		   this already!
! ::: state     <=  Scalar array
! ::: delta     => cell size
! ::: xlo,xhi   => physical locations of lower left and upper
! :::              right hand corner of grid.  (does not include
! :::		   ghost region).
! ::: -----------------------------------------------------------
subroutine ca_initdata(level,time,lo,hi,nscal, &
                       state,state_l1,state_l2,state_l3,state_h1,state_h2,state_h3, &
                       delta,xlo,xhi)

  use amrex_error_module 
  use amrex_fort_module, only : rt => amrex_real

  use network, only: nspec
  use probdata_module
  use meth_params_module, only : NVAR, URHO, UMX, UMY, UMZ, UEDEN, UEINT, UTEMP, UFS
  implicit none

  integer level, nscal
  integer lo(3), hi(3)
  integer state_l1,state_l2,state_l3,state_h1,state_h2,state_h3
  real(rt)         xlo(3), xhi(3), time, delta(3)
  real(rt)         state(state_l1:state_h1,state_l2:state_h2, &
                         state_l3:state_h3,NVAR)

  real(rt)         xcen,ycen,zcen
  integer i,j,k

  real(rt)         rsq, dist, ycloud, zcloud, ecent, denfact, rhocld

  ycloud = 0.0
  zcloud = 0.0
  !  ecent  = 1.96
  ecent  = 1.00
  denfact = 3.0246
  rhocld  = rho_r * denfact

  do k = lo(3), hi(3)
     zcen = xlo(3) + delta(3)*(float(k-lo(3)) + 0.5e0_rt)
     
     do j = lo(2), hi(2)
        ycen = xlo(2) + delta(2)*(float(j-lo(2)) + 0.5e0_rt)

        do i = lo(1), hi(1)
           xcen = xlo(1) + delta(1)*(float(i-lo(1)) + 0.5e0_rt)

           if (idir == 1) then
              if (xcen <= split(1)) then      !! left of shock
                 state(i,j,k,URHO) = rho_l
                 state(i,j,k,UMX) = rho_l*u_l
                 state(i,j,k,UMY) = 0.e0_rt
                 state(i,j,k,UMZ) = 0.e0_rt
                 state(i,j,k,UEDEN) = rhoe_l + 0.5*rho_l*u_l*u_l
                 state(i,j,k,UEINT) = rhoe_l
                 state(i,j,k,UTEMP) = T_l
              else

               rsq = (((xcen-xcloud)**2) + ((ycen-ycloud)**2) + ((zcen-zcloud)**2))/ecent
               !  rsq = ((xcen-xcloud)**2) + ((ycen-ycloud)**2) + ((zcen-zcloud)**2)/ecent
               dist = sqrt(rsq)
               if (dist .le. cldradius) then   !! inside bubble
                 state(i,j,k,URHO) = rhocld
                 state(i,j,k,UMX) = 0.e0_rt
                 state(i,j,k,UMY) = 0.e0_rt
                 state(i,j,k,UMZ) = 0.e0_rt
                 state(i,j,k,UEDEN) = rhoe_r + 0.5*rho_r*u_r*u_r
                 state(i,j,k,UEINT) = rhoe_r
                 state(i,j,k,UTEMP) = T_r
               else                            !! right of shock
                 state(i,j,k,URHO) = rho_r
                 state(i,j,k,UMX) = rho_r*u_r
                 state(i,j,k,UMY) = 0.e0_rt
                 state(i,j,k,UMZ) = 0.e0_rt
                 state(i,j,k,UEDEN) = rhoe_r + 0.5*rho_r*u_r*u_r
                 state(i,j,k,UEINT) = rhoe_r
                 state(i,j,k,UTEMP) = T_r
               endif
              endif
              
           else
              call amrex_abort('invalid idir')
           endif
 
           state(i,j,k,UFS:UFS-1+nspec) = 0.0e0_rt
           state(i,j,k,UFS  ) = state(i,j,k,URHO)

           
        enddo
     enddo
  enddo

end subroutine ca_initdata

