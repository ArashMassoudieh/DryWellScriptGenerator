#ifndef ERT_COMPARISON_H
#define ERT_COMPARISON_H

#include <string>
#include <vector>

class System;
struct model_parameters;
struct RainConfig;

template <typename T>
class TimeSeriesSet;

enum class InitThetaMode {
    Default,
    ERT3_Only,
    ERT5_Only,
    ERT_IDW_R,
    ERT_R_Avg
};

struct BoreholeSpec {
    std::string name;
    double r_m = 0.0;
    std::string obs_csv_path;
    double obs_depth_offset_m = 0.0;
};

struct BoreholeExportOptions {
    int nz_uw_n = 0;
    std::string out_dir;
    std::string model_theta_var = "theta";
    bool allow_missing_obs = false;
    bool use_resultgrid = true;
    std::string file_prefix;
    std::string file_suffix;
    unsigned int time_index = 0;
};

bool file_exists_cpp(const std::string& p);
std::string lower_copy(std::string s);
bool cli_has_init_theta(int argc, char** argv);
InitThetaMode parse_init_theta_mode(int argc, char** argv);
const char* init_theta_mode_name(InitThetaMode m);

int mapRadiusToSoilUwIndex(double r_m, const model_parameters& mp);

bool read_observed_profile_csv(const std::string& path,
                               std::vector<double>& depth_m,
                               std::vector<double>& theta_obs,
                               double depth_offset_m);

double linear_interp_or_nan(const std::vector<double>& x,
                            const std::vector<double>& y,
                            double xq);

void export_borehole_theta_comparison_csvs(const TimeSeriesSet<double>& uniformoutput,
                                           const model_parameters& mp,
                                           System* system,
                                           const std::vector<BoreholeSpec>& boreholes,
                                           const BoreholeExportOptions& opt);

std::vector<double> default_ert_measurement_times();
std::vector<BoreholeSpec> default_ert_boreholes(const std::string& workingFolder);
std::string ert_time_token(double t_excel_days);
unsigned int nearest_time_index_in_uniform_set(const TimeSeriesSet<double>& uniformoutput,
                                               double target_t_excel_days);

void export_ert_snapshots_at_measurement_times(const TimeSeriesSet<double>& uniformoutput_ERT,
                                               const model_parameters& mp,
                                               System* system,
                                               const RainConfig& raincfg,
                                               bool use_resultgrid_ert,
                                               bool allow_missing_obs);

#endif
