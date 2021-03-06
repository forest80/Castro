subroutine amrex_probinit (init,name,namlen,problo,probhi) bind(c)

  use amrex_constants_module
  use fundamental_constants_module, only: Gconst
  use actual_eos_module, only: K_const, gamma_const, polytrope_index
  use probdata_module
  use prob_params_module, only: center
  use meth_params_module, only: point_mass
  use rotation_frequency_module, only: get_omega
  use amrex_fort_module, only : rt => amrex_real

  implicit none 

  integer :: init,namlen,untin,i
  integer :: name(namlen)
  real(rt)         :: problo(3), probhi(3)
  
  namelist /fortin/ inner_radius, outer_radius, density_maximum_radius, ambient_density

  ! Build "probin" filename -- the name of file containing fortin namelist.
  integer, parameter :: maxlen = 127
  character :: probin*(maxlen)

  real(rt)         :: omega(3)
  
  do i = 1, namlen
     probin(i:i) = char(name(i))
  end do
         
  ! Read namelists in probin file
  untin = 9
  open(untin,file=probin(1:namlen),form='formatted',status='old')
  read(untin,fortin)
  close(unit=untin)

  omega = get_omega(ZERO)

  ! Figure out R_0, the maximum pressure radius.
  
  density_maximum_radius = (Gconst * point_mass / sum(omega**2))**THIRD

  ! Select the polytrope constant so that the maximum density is one.
  
  K_const = Gconst * point_mass / (TWO * (ONE + polytrope_index) * density_maximum_radius) * &
            (ONE - density_maximum_radius**2 / (inner_radius * outer_radius))

  ! Maximum and minimum vertical extent of the torus is the same as the radial extent

  torus_width = HALF * (outer_radius - inner_radius)
  torus_center = inner_radius + torus_width

end subroutine amrex_probinit



subroutine ca_initdata(level,time,lo,hi,nscal, &
                       state,state_lo,state_hi, &
                       dx,xlo,xhi)

  use amrex_constants_module
  use fundamental_constants_module, only: Gconst
  use probdata_module
  use eos_module, only: eos
  use eos_type_module, only: eos_t, eos_input_rt
  use actual_eos_module, only: polytrope_index, K_const
  use network, only: nspec
  use meth_params_module, only: NVAR, URHO, UMX, UMY, UMZ, UTEMP, UEDEN, UEINT, UFS, point_mass, do_rotation
  use rotation_frequency_module, only: get_omega
  use math_module, only: cross_product
  use prob_params_module, only: center
  use castro_util_module, only: position
  use amrex_fort_module, only : rt => amrex_real

  implicit none

  integer          :: level, nscal
  integer          :: lo(3), hi(3)
  integer          :: state_lo(3), state_hi(3)
  real(rt)         :: state(state_lo(1):state_hi(1),state_lo(2):state_hi(2),state_lo(3):state_hi(3),NVAR)
  real(rt)         :: time, dx(3)
  real(rt)         :: xlo(3), xhi(3)
  
  real(rt)         :: loc(3), vel(3), R, Z, dist
  real(rt)         :: rho, rho_s, fac
  integer          :: i, j, k

  type (eos_t)     :: eos_state

  real(rt)         :: omega(3)

  omega = get_omega(time)

  ! Rotating torus of Papaloizou and Pringle (1984), MNRAS, 208, 721.
  ! http://adsabs.harvard.edu/abs/1985MNRAS.213..799P
  ! This work is notable for discovering that rotating tori with constant
  ! specific angular momentum are unstable to non-axisymmetric perturbations.

  ! The inspiration for this problem comes from Byerly et al. (2014), ApJS, 212, 23.
  ! http://adsabs.harvard.edu/abs/2014ApJS..212...23B
  
  do k = lo(3), hi(3)
     do j = lo(2), hi(2)
        do i = lo(1), hi(1)

           loc = position(i, j, k) - center

           R = sqrt( loc(1)**2 + loc(2)**2 ) ! Cylindrical radius

           Z = loc(3)                        ! Cylindrical height

           dist = sqrt(sum(loc**2))          ! Distance from origin

           ! rho_s is the scale for the density; it satisfies that at r = density_maximum_radius, rho == 1.
           ! We guarantee this above by setting the polytropic K constant to satisfy this condition.
           ! This expression is Equation 2.9 in PP84. The constant C' can be solved to give R_0**2 / (2 * R_- * R_+),
           ! where R_0 == density_maximum_radius, R_- == inner_radius, and R_+ == outer_radius.
           ! If the term inside square brackets in Equation 2.9 is negative, that means effectively that we're no longer
           ! inside the torus, so at that point we just use a low ambient density instead.
           
           rho_s = ( Gconst * point_mass / ( (ONE + polytrope_index) * K_const * density_maximum_radius ) )**polytrope_index
           
           fac = density_maximum_radius / dist &
               - HALF * density_maximum_radius**2 / R**2 &
               - HALF * density_maximum_radius**2 / (inner_radius * outer_radius)

           if (fac > ZERO) then

              rho = rho_s * fac**polytrope_index

              if (rho < ambient_density) then
                 rho = ambient_density
              endif

           else

              rho = ambient_density

           endif

           if (rho > ambient_density .and. do_rotation .ne. 1) then
              vel = cross_product(omega, loc)
           else
              vel = ZERO
           endif
           
           eos_state % rho = rho
           eos_state % T   = 1.0 
           eos_state % xn  = 1.0 / nspec

           call eos(eos_input_rt, eos_state)

           state(i,j,k,URHO)    = rho
           state(i,j,k,UTEMP)   = eos_state % T
           state(i,j,k,UEINT)   = rho * eos_state % e

           state(i,j,k,UMX:UMZ) = rho * vel           
           state(i,j,k,UEDEN)   = state(i,j,k,UEINT) + HALF * rho * sum(vel**2)

           state(i,j,k,UFS:UFS+nspec-1) = state(i,j,k,URHO) * (ONE / nspec)
           
        enddo
     enddo
  enddo
  
end subroutine ca_initdata

