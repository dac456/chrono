// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2015 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban, Antonio Recuero
// =============================================================================
//
// Mechanism for testing tires over granular terrain.  The mechanism + tire
// system is co-simulated with a Chrono::Parallel system for the granular terrain.
//
// Definition of the RIG NODE.
//
// The global reference frame has Z up, X towards the front of the vehicle, and
// Y pointing to the left.
//
// =============================================================================

//// TODO:
////    mesh connectivity doesn't need to be communicated every time (modify Chrono?)

#ifndef TESTRIG_RIGNODE_H
#define TESTRIG_RIGNODE_H

#include <vector>

#include "chrono/physics/ChLinkLock.h"
#include "chrono/physics/ChSystemDEM.h"
#include "chrono/utils/ChUtilsInputOutput.h"

#include "chrono_fea/ChLoadContactSurfaceMesh.h"

#include "chrono_vehicle/wheeled_vehicle/tire/ANCFTire.h"

#include "BaseNode.h"

// =============================================================================

class ChFunction_SlipAngle : public chrono::ChFunction {
  public:
    ChFunction_SlipAngle(double max_angle) : m_max_angle(max_angle) {}

    virtual ChFunction_SlipAngle* Clone() const override { return new ChFunction_SlipAngle(m_max_angle); }

    virtual double Get_y(double t) const override;

  private:
    double m_max_angle;
};

// =============================================================================

class RigNode : public BaseNode {
  public:
    RigNode(double init_vel,  ///< initial wheel linear velocity
            double slip,      ///< longitudinal slip value
            int num_threads   ///< number of OpenMP threads
            );
    ~RigNode();

    /// Set body masses.
    void SetBodyMasses(double chassis_mass,  ///< mass of the (quarter-vehicle) chassis (default: 1)
                       double set_toe_mass,  ///< mass of the set-toe body (default: 1)
                       double upright_mass,  ///< mass of the upright body (default: 450)
                       double rim_mass       ///< mass of the wheel rim body (default: 15)
                       );

    /// Specify the tire JSON specification file name.
    void SetTireJSONFile(const std::string& filename);

    /// Enable/disable tire pressure (default: true).
    void EnableTirePressure(bool val);

    /// Initialize this node.
    /// This function allows the node to initialize itself and, optionally, perform an
    /// initial data exchange with any other node.
    virtual void Initialize() override;

    /// Synchronize this node.
    /// This function is called at every co-simulation synchronization time to
    /// allow the node to exchange information with any other node.
    virtual void Synchronize(int step_number, double time) override;

    /// Advance simulation.
    /// This function is called after a synchronization to allow the node to advance
    /// its state by the specified time step.  A node is allowed to take as many internal
    /// integration steps as required, but no inter-node communication should occur.
    virtual void Advance(double step_size) override;

    /// Output logging and debugging data.
    virtual void OutputData(int frame) override;

  private:
    chrono::ChSystemDEM* m_system;  ///< containing system
    bool m_constructed;             ///< system construction completed?

    std::shared_ptr<chrono::ChTimestepperHHT> m_integrator;  ///< HHT integrator object

    std::shared_ptr<chrono::ChBody> m_ground;   ///< ground body
    std::shared_ptr<chrono::ChBody> m_rim;      ///< wheel rim body
    std::shared_ptr<chrono::ChBody> m_set_toe;  ///< set toe body
    std::shared_ptr<chrono::ChBody> m_chassis;  ///< chassis body
    std::shared_ptr<chrono::ChBody> m_upright;  ///< upright body

    double m_chassis_mass;
    double m_set_toe_mass;
    double m_upright_mass;
    double m_rim_mass;

    std::string m_tire_json;                                                ///< name of tire JSON specification file
    bool m_tire_pressure;                                                   ///< tire pressure enabled?
    std::shared_ptr<chrono::vehicle::ChDeformableTire> m_tire;              ///< deformable tire
    std::shared_ptr<chrono::fea::ChLoadContactSurfaceMesh> m_contact_load;  ///< tire contact surface

    std::shared_ptr<chrono::ChLinkEngine> m_slip_motor;         ///< angular motor constraint
    std::shared_ptr<chrono::ChLinkLockPrismatic> m_prism_vel;   ///< prismatic joint for chassis linear velocity
    std::shared_ptr<chrono::ChLinkLinActuator> m_lin_actuator;  ///< actuator imposing linear velocity to system
    std::shared_ptr<chrono::ChLinkLockPrismatic> m_prism_axl;   ///< prismatic joint for chassis-axle joint
    std::shared_ptr<chrono::ChLinkEngine> m_rev_motor;          ///< motor enforcing prescribed rim angular velocity

    double m_init_vel;  ///< initial wheel forward linear velocity
    double m_slip;      ///< prescribed longitudinal slip for wheel

    // Current contact forces on tire mesh vertices
    std::vector<int> m_vert_indices;                ///< indices of vertices experiencing contact forces
    std::vector<chrono::ChVector<>> m_vert_pos;     ///< position of vertices experiencing contact forces
    std::vector<chrono::ChVector<>> m_vert_forces;  ///< contact forces on mesh vertices

    // Private methods

    void Construct();

    // Write mesh node state information
    void WriteStateInformation(chrono::utils::CSV_writer& csv);
    // Write mesh connectivity and strain information
    void WriteMeshInformation(chrono::utils::CSV_writer& csv);
    // Write contact forces on tire mesh vertices
    void WriteContactInformation(chrono::utils::CSV_writer& csv);

    void PrintLowestNode();
    void PrintLowestVertex(const std::vector<chrono::ChVector<>>& vert_pos,
                           const std::vector<chrono::ChVector<>>& vert_vel);
    void PrintContactData(const std::vector<chrono::ChVector<>>& forces, const std::vector<int>& indices);
};

#endif