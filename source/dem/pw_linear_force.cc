#include <dem/pw_linear_force.h>

using namespace dealii;

template <int dim>
void
PWLinearForce<dim>::calculate_pw_contact_force(
  const std::map<int, std::map<int, pw_contact_info_struct<dim>>>
    *                             pw_pairs_in_contact,
  const DEMSolverParameters<dim> &dem_parameters)
{
  // Defining physical properties as local variable
  const auto physical_properties = dem_parameters.physical_properties;

  // Looping over pw_pairs_in_contact, which means looping over all the active
  // particles with iterator pw_pairs_in_contact_iterator
  for (auto pw_pairs_in_contact_iterator = pw_pairs_in_contact->begin();
       pw_pairs_in_contact_iterator != pw_pairs_in_contact->end();
       ++pw_pairs_in_contact_iterator)
    {
      // Now an iterator (pw_contact_information_iterator) on each element of
      // the pw_pairs_in_contact vector is defined. This iterator iterates over
      // a map which contains the required information for calculation of the
      // contact force for each particle

      auto pairs_in_contact_content = &pw_pairs_in_contact_iterator->second;
      for (auto pw_contact_information_iterator =
             pairs_in_contact_content->begin();
           pw_contact_information_iterator != pairs_in_contact_content->end();
           ++pw_contact_information_iterator)
        {
          // Defining the iterator's second value (map value) as a local
          // parameter
          auto contact_information = pw_contact_information_iterator->second;

          // Defining total force of the contact, properties of particle as
          // local parameters
          Tensor<1, dim> total_force;
          auto           particle            = contact_information.particle;
          auto           particle_properties = particle->get_properties();

          // Calculation of effective Young's modulus of the
          // contact
          double effective_youngs_modulus =
            pow((((1.0 - pow(physical_properties.Poisson_ratio_particle, 2.0)) /
                  physical_properties.Youngs_modulus_particle) +
                 ((1.0 - pow(physical_properties.Poisson_ratio_wall, 2.0)) /
                  physical_properties.Youngs_modulus_wall)),
                -1.0);

          // Calculation of normal and tangential spring and dashpot constants
          // using particle properties
          double normal_spring_constant =
            1.0667 *
            sqrt((particle_properties[DEM::PropertiesIndex::dp] / 2.0)) *
            effective_youngs_modulus *
            pow((1.0667 * particle_properties[DEM::PropertiesIndex::mass] *
                 contact_information.normal_relative_velocity *
                 contact_information.normal_relative_velocity /
                 (sqrt((particle_properties[DEM::PropertiesIndex::dp] / 2.0)) *
                  effective_youngs_modulus)),
                0.2);

          double tangential_spring_constant =
            1.0667 *
              sqrt((particle_properties[DEM::PropertiesIndex::dp] / 2.0)) *
              effective_youngs_modulus *
              pow((1.0667 * particle_properties[DEM::PropertiesIndex::mass] *
                   contact_information.tangential_relative_velocity *
                   contact_information.tangential_relative_velocity /
                   (sqrt(
                      (particle_properties[DEM::PropertiesIndex::dp] / 2.0)) *
                    effective_youngs_modulus)),
                  0.2) +
            DBL_MIN;
          double normal_damping_constant = sqrt(
            (4 * particle_properties[DEM::PropertiesIndex::mass] *
             normal_spring_constant) /
            (1 + pow((3.1415 /
                      (log(physical_properties.restitution_coefficient_wall) +
                       DBL_MIN)),
                     2)));
          double tangential_damping_constant = sqrt(
            (4 * particle_properties[DEM::PropertiesIndex::mass] *
             tangential_spring_constant) /
            (1 + pow((3.1415 /
                      (log(physical_properties.restitution_coefficient_wall) +
                       DBL_MIN)),
                     2)));

          // Calculation of normal force using spring and dashpot normal forces
          Tensor<1, dim> spring_normal_force =
            (normal_spring_constant * contact_information.normal_overlap) *
            contact_information.normal_vector;

          Tensor<1, dim> dashpot_normal_force =
            (normal_damping_constant *
             contact_information.normal_relative_velocity) *
            contact_information.normal_vector;

          Tensor<1, dim> normal_force =
            spring_normal_force - dashpot_normal_force;

          double maximum_tangential_overlap;
          if (tangential_spring_constant > 0)
            {
              maximum_tangential_overlap =
                physical_properties.friction_coefficient_wall *
                normal_force.norm() / tangential_spring_constant;
            }
          else
            {
              maximum_tangential_overlap = 0;
            }

          // Check for gross sliding
          if (contact_information.tangential_overlap.norm() >
              maximum_tangential_overlap)
            {
              // Gross sliding occurs and the tangential overlap and tangnetial
              // force are limited to Coulumb's criterion
              contact_information.tangential_overlap =
                maximum_tangential_overlap *
                (contact_information.tangential_overlap /
                 contact_information.tangential_overlap.norm());
            }
          // Calculation of tangential force using spring and dashpot tangential
          // forces
          Tensor<1, dim> spring_tangential_force =
            tangential_spring_constant * contact_information.tangential_overlap;
          Tensor<1, dim> dashpot_tangential_force =
            tangential_damping_constant *
            contact_information.tangential_relative_velocity;
          Tensor<1, dim> tangential_force =
            -1.0 * spring_tangential_force - dashpot_tangential_force;

          // Calculation of total force
          total_force = normal_force + tangential_force;

          // Updating the body force of particles in the particle handler
          for (int d = 0; d < dim; ++d)
            {
              particle_properties[DEM::PropertiesIndex::force_x + d] =
                particle_properties[DEM::PropertiesIndex::force_x + d] +
                total_force[d];
            }

          // Calculation of torque
          // First calculation of torque due to tangential force acting on
          // particle
          Tensor<1, dim> tangential_toruqe;
          if (dim == 3)
            ((particle_properties[DEM::PropertiesIndex::dp]) / 2.0) *
              cross_product_3d(contact_information.normal_vector, total_force);

          // Getting the angular velocity of particle in the vector format
          Tensor<1, dim> angular_velocity;
          for (int d = 0; d < dim; ++d)
            {
              angular_velocity[d] =
                particle_properties[DEM::PropertiesIndex::omega_x + d];
            }

          // Calculation of particle-wall angular velocity (norm of the
          // particle angular velocity)
          Tensor<1, dim> pw_angular_velocity;
          for (int d = 0; d < dim; ++d)
            {
              pw_angular_velocity[d] = 0.0;
            }

          double omega_value = angular_velocity.norm();
          if (omega_value != 0)
            {
              pw_angular_velocity = angular_velocity / omega_value;
            }

          // Calcualation of rolling resistance torque
          Tensor<1, dim> rolling_resistance_torque =
            -1.0 * physical_properties.rolling_friction_wall *
            ((particle_properties[DEM::PropertiesIndex::dp]) / 2.0) *
            normal_force.norm() * pw_angular_velocity;

          // Updating the acting toruqe on the particle
          for (int d = 0; d < dim; ++d)
            {
              particle_properties[DEM::PropertiesIndex::M_x + d] =
                particle_properties[DEM::PropertiesIndex::M_x + d] +
                tangential_toruqe[d] + rolling_resistance_torque[d];
            }
        }
    }
}

template class PWLinearForce<2>;
template class PWLinearForce<3>;
