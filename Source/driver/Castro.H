
#ifndef _Castro_H_
#define _Castro_H_

#include <AMReX_BC_TYPES.H>
#include <AMReX_AmrLevel.H>
#include <AMReX_iMultiFab.H>
#include <AMReX_ErrorList.H>
#include <AMReX_FluxRegister.H>

#ifdef BL_LAZY
#include <AMReX_Lazy.H>
#endif

#ifdef AMREX_PARTICLES
#include "AMReX_AmrParticles.H"
#endif

#include <memory>
#include <iostream>

using std::istream;
using std::ostream;


enum StateType { State_Type = 0,
#ifdef RADIATION
                 Rad_Type,
#endif
#ifdef SELF_GRAVITY
                 PhiGrav_Type,
                 Gravity_Type,
#endif
#ifdef ROTATION
		 PhiRot_Type,
		 Rotation_Type,
#endif
		 Source_Type,
#ifdef REACTIONS
                 Reactions_Type,
#endif
#ifdef SDC
#ifdef REACTIONS
		 SDC_React_Type
#endif
#endif
};

// Create storage for all source terms.

enum sources { ext_src = 0,
               thermo_src,
#ifdef SPONGE
               sponge_src,
#endif
#ifdef DIFFUSION
               diff_src,
#endif
#ifdef HYBRID_MOMENTUM
               hybrid_src,
#endif
#ifdef GRAVITY
               grav_src,
#endif
#ifdef ROTATION
               rot_src,
#endif
	       num_src };

//
// AmrLevel-derived class for hyperbolic conservation equations for stellar media
//

class Castro
    :
    public amrex::AmrLevel
{
public:
    //
    //Default constructor.  Builds invalid object.
    //
    Castro ();
    //
    //The basic constructor.
    //
    Castro (amrex::Amr&            papa,
            int             lev,
            const amrex::Geometry& level_geom,
            const amrex::BoxArray& bl,
            const amrex::DistributionMapping& dm,
            amrex::Real            time);
    //
    //The destructor.
    //
    virtual ~Castro () override;
    //
    //Restart from a checkpoint file.
    //
    virtual void restart (amrex::Amr&     papa,
                          istream& is,
			  bool bReadSpecial = false) override;
    //
    //This is called only when we restart from an old checkpoint.
    //
    virtual void set_state_in_checkpoint (amrex::Vector<int>& state_in_checkpoint) override;
    //
    //Call amrex::AmrLevel::checkPoint and then add radiation info
    //
    virtual void checkPoint(const std::string& dir,
                            std::ostream&      os,
                            amrex::VisMF::How         how,
                            bool               dump_old) override;

    /*A string written as the first item in writePlotFile() at
               level zero. It is so we can distinguish between different
               types of plot files. For Castro it has the form: Castro-Vnnn
    */
    virtual std::string thePlotFileType () const override;

    virtual void setPlotVariables () override;
    //
    //Write a plotfile to specified directory.
    //
    virtual void writePlotFile (const std::string& dir,
                                ostream&       os,
                                amrex::VisMF::How     how) override;
    virtual void writeSmallPlotFile (const std::string& dir,
				     ostream&       os,
				     amrex::VisMF::How     how) override;
    void plotFileOutput(const std::string& dir,
                        ostream& os,
                        amrex::VisMF::How how,
                        const int is_small);

    void writeJobInfo (const std::string& dir);

    //
    // Define data descriptors.
    //
    static void variableSetUp ();
    //
    // Define tagging functions.
    //
    static void ErrorSetUp ();
    //
    // Cleanup data descriptors at end of run.
    //
    static void variableCleanUp ();
    //
    // Initialize grid data at problem start-up.
    //
    virtual void initData () override;

#ifdef AMREX_PARTICLES
    //
    // Read particle-related inputs
    //
    static void read_particle_params ();
    //
    // Initialize particle locations and velocities (and strengths if relevant)
    //
    void init_particles ();
    //
    // Write particles in checkpoint directories
    //
    void ParticleCheckPoint (const std::string& dir);
    //
    // Write particles in plotfile directories
    //
    void ParticlePlotFile (const std::string& dir);
    //
    // How to initialize at restart
    //
    void ParticlePostRestart (const std::string& restart_file);
    //
    // Derived quantities associated with particles
    //
    std::unique_ptr<amrex::MultiFab> ParticleDerive (const std::string& name,
						     amrex::Real           time,
						     int            ngrow);
    //
    // Timestamp particles
    //
    void TimestampParticles (int ngrow);
    //
    // Advance the particles by dt
    //
    void advance_particles (int iteration, amrex::Real time, amrex::Real dt);

#endif

#ifdef MAESTRO_INIT
    void MAESTRO_init ();
#endif

    //
    // Get problem metrics.
    //
    amrex::MultiFab* Area ();
    amrex::MultiFab& Area (int dir);
    //
    // The volume of the multifab.
    //
    amrex::MultiFab& Volume ();
    //
    // Return the n'th fluxes MultiFab.
    //
    amrex::MultiFab& Fluxes (int dir);
    //
    // Set time levels of state data.
    //
    virtual void setTimeLevel (amrex::Real time,
                               amrex::Real dt_old,
                               amrex::Real dt_new) override;
    //
    // Initialize data on this level from another Castro (during regrid).
    //
    virtual void init (amrex::AmrLevel& old) override;
    //
    // Initialize data on this level after regridding if old level did not previously exist
    //
    virtual void init () override;
    //
    // Proceed with next timestep?
    //
    virtual int okToContinue () override;
    //
    // Advance grids at this level in time.
    //
    virtual amrex::Real advance (amrex::Real time,
                          amrex::Real dt,
                          int  iteration,
                          int  ncycle) override;

#ifndef AMREX_USE_CUDA
    amrex::Real do_advance (amrex::Real time,
                     amrex::Real dt,
                     int  amr_iteration,
                     int  amr_ncycle);
#endif

    amrex::Real do_advance_mol (amrex::Real time,
                     amrex::Real dt,
                     int  amr_iteration,
                     int  amr_ncycle);

#ifndef AMREX_USE_CUDA
    bool retry_advance (amrex::Real& time, amrex::Real dt, int amr_iteration, int amr_ncycle);

    amrex::Real subcycle_advance (amrex::Real time, amrex::Real dt, int amr_iteration, int amr_ncycle);
#endif

    void initialize_advance(amrex::Real time, amrex::Real dt, int amr_iteration, int amr_ncycle);

    void finalize_advance(amrex::Real time, amrex::Real dt, int amr_iteration, int amr_ncycle);

    void initialize_do_advance(amrex::Real time, amrex::Real dt, int amr_iteration, int amr_ncycle);

    void finalize_do_advance(amrex::Real time, amrex::Real dt, int amr_iteration, int amr_ncycle);

    void cons_to_prim(const amrex::Real time);

#ifndef AMREX_USE_CUDA
    void cons_to_prim_fourth(const amrex::Real time);
#endif

    void check_for_cfl_violation(const amrex::Real dt);

#ifndef AMREX_USE_CUDA
    void construct_hydro_source(amrex::Real time, amrex::Real dt);
#endif

    void construct_mol_hydro_source(amrex::Real time, amrex::Real dt);

    void check_for_nan(amrex::MultiFab& state, int check_ghost=0);

#ifdef SDC
    void cons_to_prim(amrex::MultiFab& u, amrex::MultiFab& q, amrex::MultiFab& qaux);
#endif

    bool source_flag(int src);

    bool apply_sources();

    static int get_output_at_completion();

    void do_old_sources(amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt, int amr_iteration = -1, int amr_ncycle = -1);

    void construct_old_source(int src, amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt, int amr_iteration = -1, int amr_ncycle = -1);

    void do_new_sources(amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt, int amr_iteration = -1, int amr_ncycle = -1);

    void construct_new_source(int src, amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt, int amr_iteration = -1, int amr_ncycle = -1);

    amrex::Vector<amrex::Real> evaluate_source_change(amrex::MultiFab& update, amrex::Real dt, bool local = false);

    void print_source_change(amrex::Vector<amrex::Real> update);

    void print_all_source_changes(amrex::Real dt, bool is_new);

    void sum_of_sources(amrex::MultiFab& source);

    void time_center_source_terms (amrex::MultiFab& S_new,
				   amrex::MultiFab& src_old,
				   amrex::MultiFab& src_new,
				   amrex::Real dt = 1.0);

#ifdef SPONGE
    void construct_old_sponge_source(amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt);

    void construct_new_sponge_source(amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt);

    static void sponge_init();

    static void sponge_finalize();
#endif

    void construct_old_ext_source(amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt);

    void construct_new_ext_source(amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt);

    void fill_ext_source(amrex::Real time, amrex::Real dt, amrex::MultiFab& S_old, amrex::MultiFab& S_new, amrex::MultiFab& ext_src);

    void construct_old_thermo_source(amrex::MultiFab& source, amrex::MultiFab& state,
                                     amrex::Real time, amrex::Real dt);

    void construct_new_thermo_source(amrex::MultiFab& source, amrex::MultiFab& state_old,
                                     amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt);

    void fill_thermo_source(amrex::Real time, amrex::Real dt,
                            amrex::MultiFab& S_old, amrex::MultiFab& S_new,
                            amrex::MultiFab& ext_src);

#ifdef GRAVITY
#ifdef SELF_GRAVITY
    void construct_old_gravity(int amr_iteration, int amr_ncycle,
			       amrex::Real time);

    void construct_new_gravity(int amr_iteration, int amr_ncycle,
			       amrex::Real time);
#endif
    void construct_old_gravity_source(amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt);

    void construct_new_gravity_source(amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt);
#endif

    //
    // Estimate time step.
    //
    amrex::Real estTimeStep (amrex::Real dt_old);
    //
    // Compute initial time step.
    //
    amrex::Real initialTimeStep ();
    //
    // Compute initial `dt'.
    //
    virtual void computeInitialDt (int                   finest_level,
                                   int                   sub_cycle,
                                   amrex::Vector<int>&           n_cycle,
                                   const amrex::Vector<amrex::IntVect>& ref_ratio,
                                   amrex::Vector<amrex::Real>&          dt_level,
                                   amrex::Real                  stop_time) override;
    //
    // Compute new `dt'.
    //
    virtual void computeNewDt (int                   finest_level,
                               int                   sub_cycle,
                               amrex::Vector<int>&           n_cycle,
                               const amrex::Vector<amrex::IntVect>& ref_ratio,
                               amrex::Vector<amrex::Real>&          dt_min,
                               amrex::Vector<amrex::Real>&          dt_level,
                               amrex::Real                  stop_time,
                               int                   post_regrid_flag) override;
    //
    // Allocate data at old time.
    //
    virtual void allocOldData () override;
    //
    // Remove data at old time.
    //
    virtual void removeOldData () override;
    //
    // Passes some data about the grid to a Fortran module.
    //
    void setGridInfo();
    //
    // Print information about energy budget.
    //
    void do_energy_diagnostics();
    //
    // Do work after timestep().
    //
    virtual void post_timestep (int iteration) override;
    //
    // Contains operations to be done only after a full coarse timestep.
    //
    virtual void postCoarseTimeStep (amrex::Real cumtime) override;
    //
    // Check whether we want to do a regrid.
    //
    void check_for_post_regrid (amrex::Real time);
    //
    // Do work after regrid().
    //
    virtual void post_regrid (int lbase,
                              int new_finest) override;
    //
    // Do work after a restart().
    //
    virtual void post_restart () override;
    //
    // Do work after init().
    //
    virtual void post_init (amrex::Real stop_time) override;
    //
    // Do work after restart with grown_factor > 1
    //
    void post_grown_restart ();

#ifdef DO_PROBLEM_POST_SIMULATION
    // Do work at the end of the simulation -- before the last outputs
    static void problem_post_simulation (amrex::Vector<std::unique_ptr<AmrLevel> >& amr_level);
#endif

    //
    // Error estimation for regridding.
    //
    virtual void errorEst (amrex::TagBoxArray& tb,
                           int          clearval,
                           int          tagval,
                           amrex::Real  time,
			   int          n_error_buf = 0,
			   int          ngrow = 0) override;

    //
    // Apply the set_problem_tags routine.
    //
    void apply_problem_tags (amrex::TagBoxArray& tags, int clearval, int setval, amrex::Real time);

    //
    // Apply a given tagging function.
    //
    void apply_tagging_func (amrex::TagBoxArray& tags, int clearval, int setval, amrex::Real time, int j);

    //
    // Hooks for problem-specific operations before and after applying custom tagging.
    //
    void problem_pre_tagging_hook (amrex::TagBoxArray& tags, int clearval, int tagval, amrex::Real time);
    void problem_post_tagging_hook (amrex::TagBoxArray& tags, int clearval, int tagval, amrex::Real time);

    // Returns a MultiFab containing the derived data for this level.
    // If ngrow>0 the MultiFab is built on the appropriately grown BoxArray.
    virtual std::unique_ptr<amrex::MultiFab> derive (const std::string& name,
						     amrex::Real        time,
						     int                ngrow) override;

    // This version of derive() fills the dcomp'th component of mf with the derived quantity.
    virtual void derive (const std::string& name,
			 amrex::Real               time,
			 amrex::MultiFab&          mf,
			 int                dcomp) override;

    static int numGrow();

#ifdef REACTIONS
#ifndef SDC
    void react_state(amrex::MultiFab& state,
		     amrex::MultiFab& reactions,
		     const amrex::iMultiFab& mask,
		     amrex::MultiFab& weights,
		     amrex::Real time,
		     amrex::Real dt_react,
		     int strang_half, int  ngrow = 0);

    void strang_react_first_half(amrex::Real time, amrex::Real dt);

    void strang_react_second_half(amrex::Real time, amrex::Real dt);
#else
    void react_state(amrex::Real time, amrex::Real dt);
    void get_react_source_prim(amrex::MultiFab& source, amrex::Real dt);
#endif

    void strang_chem (amrex::MultiFab&  state,
                      amrex::MultiFab&  react_mf,
                      amrex::Real       time,
                      amrex::Real       dt);

    bool valid_zones_to_burn(amrex::MultiFab& State);
#endif

#ifdef ROTATION
    void construct_old_rotation_source(amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt);

    void construct_new_rotation_source(amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt);

    void fill_rotation_field(amrex::MultiFab& phi, amrex::MultiFab& rot, amrex::MultiFab& state, amrex::Real time);
#endif

#ifdef RADIATION
    void final_radiation_call (amrex::MultiFab& S_new,
                               int iteration,
                               int ncycle);
#endif

    void reset_internal_energy (amrex::MultiFab& State);

    void computeTemp (amrex::MultiFab& State, int ng);

    void apply_source_term_predictor();

    void swap_state_time_levels (const amrex::Real dt);

#ifdef DIFFUSION
    void construct_old_diff_source(amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt);
    void construct_new_diff_source(amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt);

    void getTempDiffusionTerm (amrex::Real time, amrex::MultiFab& state, amrex::MultiFab& DiffTerm);
    void getEnthDiffusionTerm (amrex::Real time, amrex::MultiFab& state, amrex::MultiFab& DiffTerm);
#if (BL_SPACEDIM == 1)
    void getSpecDiffusionTerm (amrex::Real time, amrex::MultiFab& state, amrex::MultiFab& DiffTerm);
#endif
#if (BL_SPACEDIM == 1)
    void getViscousTerm (amrex::Real time, amrex::MultiFab& state, amrex::MultiFab& ViscousTermforMomentum, amrex::MultiFab& ViscousTermforEnergy);
    void getFirstViscousTerm (amrex::Real time, amrex::MultiFab& state, amrex::MultiFab& ViscousTerm);
    void getSecndViscousTerm (amrex::Real time, amrex::MultiFab& state, amrex::MultiFab& ViscousTerm);
    void getViscousTermForEnergy (amrex::Real time, amrex::MultiFab& state, amrex::MultiFab& ViscousTermforEnergy);
#endif
    void add_temp_diffusion_to_source (amrex::MultiFab& ext_src, amrex::MultiFab& source, amrex::MultiFab& DiffTerm, amrex::Real t, amrex::Real mult_factor = 1.0);
#if (BL_SPACEDIM == 1)
    void add_spec_diffusion_to_source (amrex::MultiFab& ext_src, amrex::MultiFab& source, amrex::MultiFab& DiffTerm, amrex::Real t, amrex::Real mult_factor = 1.0);
#endif
#if (BL_SPACEDIM == 1)
    void add_viscous_term_to_source   (amrex::MultiFab& ext_src, amrex::MultiFab& state, amrex::MultiFab& ViscousTermforMomentum,
                                       amrex::MultiFab& ViscousTermforEnergy, amrex::Real t, amrex::Real mult_factor = 1.0);
#endif
#endif

#ifdef ROTATION
  void add_rotation_to_source(amrex::MultiFab& ext_src, amrex::MultiFab& RotationTerm, amrex::Real old_time);
  void time_center_rotation(amrex::MultiFab& S_new, amrex::MultiFab& OldRotationTerm, amrex::Real cur_time, amrex::Real dt);
#endif

    amrex::Real volWgtSum (const std::string& name, amrex::Real time, bool local=false, bool finemask=true);

    amrex::Real volWgtSquaredSum (const std::string& name, amrex::Real time, bool local=false);

    amrex::Real locWgtSum (const std::string& name, amrex::Real time, int idir, bool local=false);

    amrex::Real locWgtSum2D (const std::string& name, amrex::Real time, int idir1, int idir2, bool local=false);

    amrex::Real volWgtSumMF (const amrex::MultiFab& mf, int comp, bool local=false);

    amrex::Real locWgtSumOneSide (const std::string& name, amrex::Real time, int idir, int side, int bdir, bool local=false);

    amrex::Real volWgtSumOneSide (const std::string& name, amrex::Real time, int side, int bdir, bool local=false);

    amrex::Real volProductSum (const std::string& name1, const std::string& name2, amrex::Real time, bool local=false);

    amrex::Real locSquaredSum (const std::string& name, amrex::Real time, int idir, bool local=false);

#ifdef POINTMASS
    int using_point_mass ();
    amrex::Real get_point_mass ();
#endif

#ifdef HYBRID_MOMENTUM
    void construct_old_hybrid_source(amrex::MultiFab& source, amrex::MultiFab& state, amrex::Real time, amrex::Real dt);

    void construct_new_hybrid_source(amrex::MultiFab& source, amrex::MultiFab& state_old, amrex::MultiFab& state_new, amrex::Real time, amrex::Real dt);

    void fill_hybrid_hydro_source(amrex::MultiFab& state, amrex::MultiFab& source, const amrex::Real mult_factor);

    void hybrid_sync(amrex::MultiFab& state);
#endif

    void add_force_to_sources (amrex::MultiFab& force, amrex::MultiFab& sources, amrex::MultiFab& state);

    void apply_source_to_state (int is_new, amrex::MultiFab& state, amrex::MultiFab& source, amrex::Real dt, int ng = 0);

    void expand_state(amrex::MultiFab& S, amrex::Real time, int iclean, int ng);

#ifdef SELF_GRAVITY
    void make_radial_data (int is_new);
#endif

#ifdef AUX_UPDATE
    void advance_aux(amrex::Real time, amrex::Real dt);
#endif

#ifdef AMREX_PARTICLES
    static amrex::AmrTracerParticleContainer* theTracerPC () { return TracerPC; }
#endif

    static int       NUM_STATE;

    // conservative variable index keys
    static int       Density, Xmom, Ymom, Zmom, Eden, Eint, Temp;
#ifdef HYBRID_MOMENTUM
    static int       Rmom, Lmom, Pmom;
#endif
#ifdef SHOCK_VAR
    static int       Shock;
#endif

    static int       FirstAdv,  NumAdv;
    static int       FirstSpec, NumSpec;
    static int       FirstAux,  NumAux;

    // primitive variable index keys
    static int       QRHO, QU, QV, QW;
    static int       QGAME, QPRES, QREINT, QTEMP;
    static int       QFA, QFS, QFX;
#ifdef MHD
    static int       QMAGX, QMAGY, QMAGZ;
#endif
#ifdef RADIATION
    static int       QPTOT, QREITOT, QRAD;
#endif

    // variable counts
    static int       QVAR;
    static int       NQAUX;
    static int       NQ;
    static int       NGDNV;


    static amrex::Vector<std::string> source_names;

    //
    // This MultiFab is on the coarser level.  This is useful for the coarser level
    // to mask out the finer level.  We only build this when it is needed.
    // This coarse MultiFab has to live on the fine level because it must be updated
    // even when only the fine level changes.
    //
    amrex::MultiFab fine_mask;
    amrex::MultiFab& build_fine_mask();

    //
    // A record of how many cells we have advanced throughout the simulation.
    // This is saved as a real because we will be storing the number of zones
    // advanced as a ratio with the number of zones on the coarse grid (to
    // help prevent the number from getting too large), and that may be a
    // non-integer number.
    //
    static amrex::Real num_zones_advanced;

protected:

    //
    // Build a mask that ghost cells overlapping with interior cells in the same multifab
    // are set to 0, whereas others are set to 1.
    //
    amrex::Vector<std::unique_ptr<amrex::iMultiFab> > ib_mask;
    amrex::iMultiFab& build_interior_boundary_mask (int ng);

#ifdef SELF_GRAVITY
    int get_numpts();

    //
    // Difference between composite and level solve for phi.
    //
    amrex::MultiFab comp_minus_level_phi;
    amrex::Vector<std::unique_ptr<amrex::MultiFab> > comp_minus_level_grad_phi;
#endif

    //
    // A state array with ghost zones.
    //
    amrex::MultiFab Sborder;

#ifdef RADIATION
    amrex::MultiFab Erborder;
    amrex::MultiFab lamborder;
#endif

    //
    // A state array for the post burn state to be used in MOL integration
    //
    amrex::MultiFab Sburn;

    //
    // The primitive variable state array.
    //
    amrex::MultiFab q;

    // we need a second version when doing fourth order
    amrex::MultiFab q_bar;

    //
    // The auxiliary primitive variable state array.
    //
    amrex::MultiFab qaux;

    //
    // The source terms in primitive form.
    //
    amrex::MultiFab src_q;

    //
    // Source terms to the hydrodynamics solve.
    //
    amrex::MultiFab sources_for_hydro;

    //
    // Source term representing hydrodynamics update -- this should just be
    // -div{F(U)} -- no source terms
    //
    amrex::MultiFab hydro_source;

    //
    // Hydrodynamic (and radiation) fluxes.
    //
    amrex::Vector<std::unique_ptr<amrex::MultiFab> > fluxes;
#if (BL_SPACEDIM <= 2)
    amrex::MultiFab         P_radial;
#endif
#ifdef RADIATION
    amrex::Vector<std::unique_ptr<amrex::MultiFab> > rad_fluxes;
#endif

    amrex::Vector<std::unique_ptr<amrex::MultiFab> > mass_fluxes;

    amrex::FluxRegister flux_reg;
#if (BL_SPACEDIM <= 2)
    amrex::FluxRegister pres_reg;
#endif
#ifdef RADIATION
    amrex::FluxRegister rad_flux_reg;
#endif
#ifdef GRAVITY
    amrex::FluxRegister phi_reg;
#endif

    // Scalings for the flux registers.
    amrex::Real flux_crse_scale;
    amrex::Real flux_fine_scale;

#if (BL_SPACEDIM <= 2)
    amrex::Real pres_crse_scale;
    amrex::Real pres_fine_scale;
#endif

    //
    // Did we trigger a CFL violation?
    //
    int cfl_violation;

    //
    // Did we have a successful burn?
    //
    int burn_success;

    //
    // State data to hold if we want to do a retry.
    //
    amrex::Vector<std::unique_ptr<amrex::StateData> > prev_state;

    //
    // Save the simulation time of the prev_state.
    //
    amrex::Real prev_state_old_time;
    amrex::Real prev_state_new_time;
    bool prev_state_had_old_data;

    //
    // Flag for indicating that we want to save prev_state until the reflux.
    //
    bool keep_prev_state;

    //
    // Storage for the method of lines stages
    amrex::Vector<std::unique_ptr<amrex::MultiFab> > k_mol;

    // MOL Butcher tableau
    static int MOL_STAGES;

    static amrex::Vector< amrex::Vector<amrex::Real> > a_mol;
    static amrex::Vector<amrex::Real> b_mol;
    static amrex::Vector<amrex::Real> c_mol;

    // Wall time that we started the timestep
    amrex::Real wall_time_start;

    //
    //  Call extern/networks/.../network.f90::network_init()
    //
    static void extern_init ();

    static void network_init ();

    static void network_finalize ();

    static void eos_finalize ();

    static void amrinfo_init();

    static void amrinfo_finalize();

    static void read_params ();

    Castro& getLevel (int lev);

    void FluxRegCrseInit();
    void FluxRegFineAdd();

    void reflux (int crse_level, int fine_level);

    void normalize_species (amrex::MultiFab& S_new, int ng);

    void enforce_consistent_e (amrex::MultiFab& S);

    amrex::Real enforce_min_density (amrex::MultiFab& S_old, amrex::MultiFab& S_new, int ng);

    amrex::Real clean_state (int is_new, amrex::MultiFab& state, int ng);
    amrex::Real clean_state (int is_new, int ng);

    amrex::Real clean_state (amrex::MultiFab& state);

    void avgDown ();

    void avgDown (int state_indx);

    void buildMetrics ();

    void initMFs ();

    // integrate derived quantities over domain

    amrex::Real sumDerive (const std::string& name, amrex::Real time, bool local=false);

    void sum_integrated_quantities ();

    void write_info ();

#ifdef SELF_GRAVITY
    void define_new_center (amrex::MultiFab& S, amrex::Real time);
    void write_center ();
#endif

#ifdef POINTMASS
    void pointmass_update(amrex::Real time, amrex::Real dt);
#endif

    void stopJob ();

    //
    // The data.
    //
    amrex::MultiFab             volume;
    amrex::MultiFab             area[3];
    amrex::MultiFab             dLogArea[1];
    amrex::Vector< amrex::Vector<amrex::Real> > radius;

    //
    // Keep track of which AMR iteration we're on.
    //
    int iteration;

    //
    // Static data members.
    //
#include <castro_params.H>

#ifdef AMREX_PARTICLES
#include "particles_params.H"
#endif

    static bool      signalStopJob;
    static bool      dump_old;
    static int       radius_grow;
    static int       verbose;
    static amrex::ErrorList err_list;
    static int              num_err_list_default;
    static amrex::BCRec     phys_bc;
    static int       NUM_GROW;

    static int         lastDtPlotLimited;
    static amrex::Real lastDtBeforePlotLimiting;

    int lastDtRetryLimited;
    amrex::Real lastDtFromRetry;

    // for keeping track of mass changes from negative density resets
    static amrex::Real      frac_change;

    // For keeping track of fluid quantities lost at physical grid boundaries.
    // This should persist through restarts, but right now only on level 0.
    static const int n_lost = 8;
    amrex::Real             material_lost_through_boundary_cumulative[n_lost];
    amrex::Real             material_lost_through_boundary_temp[n_lost];

    // for keeping track of the amount of CPU time used -- this will persist
    // after restarts
    static amrex::Real      previousCPUTimeUsed;
    static amrex::Real      startCPUTime;

    static amrex::Real getCPUTime();

    bool             FillPatchedOldState_ok;

    // There can be only one Gravity object, it covers all levels:
    static class Gravity *gravity;

    // There can be only one Diffusion object, it covers all levels:
    static class Diffusion *diffusion;

#ifdef RADIATION
    // permits radiation to be turned on and off without recompiling:
    static int do_radiation;

    // There can be only one Radiation object, it covers all levels:
    static class Radiation *radiation;

    friend class Radiation;
#endif

#ifdef AMREX_PARTICLES
    static amrex::AmrTracerParticleContainer* TracerPC;
#endif

    // Name of the probin file and its length.
    static std::string probin_file;

    static amrex::IntVect hydro_tile_size;
    static amrex::IntVect no_tile_size;

    static int Knapsack_Weight_Type;
    static int num_state_type;


    // counters for various retries in Castro

    // subcycle capability
    int sub_iteration;
    int sub_ncycle;
    bool do_subcycle;
    amrex::Real dt_subcycle;
    amrex::Real dt_advance;

    // sdc
    int sdc_iteration;

    // method of lines
    int mol_iteration;

/* problem-specific includes */
#include <Problem.H>

};

//
// Inlines.
//

inline
int
Castro::numGrow()
{
    return NUM_GROW;
}

inline
amrex::MultiFab*
Castro::Area ()
{
    return area;
}

inline
amrex::MultiFab&
Castro::Area (int dir)
{
    return area[dir];
}

inline
amrex::MultiFab&
Castro::Volume ()
{
    return volume;
}

inline
Castro&
Castro::getLevel (int lev)
{
    return *(Castro *) &parent->getLevel(lev);
}

inline
void
Castro::stopJob()
{
  signalStopJob = true;


}

inline
int
Castro::get_output_at_completion()
{
    return output_at_completion;
}

#ifdef POINTMASS
inline
amrex::Real
Castro::get_point_mass ()
{
    return point_mass;
}

inline
int
Castro::using_point_mass ()
{
    return use_point_mass;
}
#endif


#endif /*_Castro_H_*/
