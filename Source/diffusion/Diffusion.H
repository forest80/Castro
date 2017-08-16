#ifndef _Diffusion_H_
#define _Diffusion_H_

#include <AMReX_AmrLevel.H>

class Diffusion {

public:

  Diffusion(amrex::Amr* Parent, amrex::BCRec* _phys_bc);
  ~Diffusion ();
  void read_params ();

  void install_level (int                   level,
                      amrex::AmrLevel*             level_data,
                      amrex::MultiFab&             volume,
                      amrex::MultiFab*             area);

  void applyop(int level,amrex::MultiFab& Temperature,amrex::MultiFab& CrseTemp,
               amrex::MultiFab& DiffTerm, amrex::Array<std::unique_ptr<amrex::MultiFab> >& temp_cond_coef);

  void applyViscOp(int level,amrex::MultiFab& Vel, amrex::MultiFab& CrseVel,
                   amrex::MultiFab& ViscTerm, amrex::Array<std::unique_ptr<amrex::MultiFab> >& visc_coeff);

  void make_mg_bc();

  void GetCrsePhi(int level, 
                  amrex::MultiFab& phi_crse,
                  amrex::Real time);

  void zeroPhiFluxReg (int level);


protected:
  //
  // Pointers to amr,amrlevel.
  //
  amrex::Amr*             parent;
  amrex::Array<amrex::AmrLevel*> LevelData;

  //
  // BoxArray at each level
  //
  amrex::Array<amrex::BoxArray> grids;
  //
  // Volume and area fractions.
  //
  amrex::Array<amrex::MultiFab*> volume;
  amrex::Array<amrex::MultiFab*> area;

  int mg_bc[2*BL_SPACEDIM];

  amrex::BCRec*       phys_bc;

#include "diffusion_params.H"

  static int   stencil_type;

#if (BL_SPACEDIM < 3)
  void applyMetricTerms(int level,amrex::MultiFab& Rhs, amrex::Array<std::unique_ptr<amrex::MultiFab> >& coeffs);
  void   weight_cc(int level,amrex::MultiFab& cc);
  void unweight_cc(int level,amrex::MultiFab& cc);
#endif
};
#endif
