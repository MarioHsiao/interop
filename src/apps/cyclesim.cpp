/** @page apps Applications
 *
 * @brief Write out InterOps for all entries up to a certain cycle
 *
 * This application writes out a new set of binary InterOp files for all records up to a specific cycle.
 *
 * Running the Program
 * -------------------
 *
 * The program runs as follows:
 *
 *      $ cyclesim 140131_1287_0851_A01n401drr ./ 26 1
 *
 * In this sample, 140131_1287_0851_A01n401drr is a run folder than contains a sub directory called InterOp, `./` write
 * to the current directory, 26 is the maximum number of cycles for the records and 1 is the maximum number of reads.
 * The program will output a directory called ./140131_1287_0851_A01n401drr_MaxCycle_26.
 *
 * The InterOp sub folder may contain any of the following files:
 *
 *  - CorrectedIntMetricsOut.bin
 *  - ErrorMetricsOut.bin
 *  - ExtractionMetricsOut.bin
 *  - ImageMetricsOut.bin
 *  - IndexMetricsOut.bin
 *  - QMetricsOut.bin
 *  - TileMetricsOut.bin
 *
 * Error Handling
 * --------------
 *
 *  The `cyclesim` program will print an error to the error stream and return an error code (any number except 0)
 *  when an error occurs. There are two likely errors that may arise:
 *
 *      1. The InterOp path was incorrect
 *      2. The InterOp files do not contain valid data
 *
 *  A missing InterOp file will be silently ignored. Incomplete InterOp files are also ignored.
 */
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include "interop/util/lexical_cast.h"
#include "interop/util/filesystem.h"
#include "interop/io/metric_file_stream.h"
#include "interop/model/metric_sets/tile_metric_set.h"
#include "interop/model/metric_sets/error_metric_set.h"
#include "interop/model/metric_sets/corrected_intensity_metric_set.h"
#include "interop/model/metric_sets/extraction_metric_set.h"
#include "interop/model/metric_sets/image_metric_set.h"
#include "interop/model/metric_sets/q_metric_set.h"
#include "interop/version.h"

using namespace illumina::interop::model::metrics;
using namespace illumina::interop;

/** Exit codes that can be produced by the application
 */
enum exit_codes
{
    /** The program exited cleanly, 0 */
    SUCCESS,
    /** Invalid arguments were given to the application*/
    INVALID_ARGUMENTS,
    /** Empty InterOp directory*/
    NO_INTEROPS_FOUND,
    /** InterOp file has a bad format */
    BAD_FORMAT,
    /** Unknown error has occurred*/
    UNEXPECTED_EXCEPTION,
    /** InterOp file has not records */
    EMPTY_INTEROP
};
/** Write a help message to the output stream
 *
 * @param out output stream
 */
void print_help(std::ostream& out);
/** Read all the metrics and write them to the output stream
 *
 * This reads metrics from the binary interop and writes to the output stream in the following order:
 *  1. Tile
 *  2. Error
 *  3. Corrected Intensity
 *  4. Extraction
 *  5. Image
 *  6. Q
 *  7. Index
 *
 * @param input path to run folder
 * @param output path to output run folder
 * @param max_cycle number of cycles to copy
 * @param max_read number of reads to copy
 * @param cycle_to_align cycle alignment occurs
 * @return error code or 0
 */
int write_interops(const std::string& input, const std::string& output, unsigned int max_cycle, unsigned int max_read, unsigned int cycle_to_align);

/** Set false if you want to disable error messages printing to the error stream */
bool kPrintError=true;
/** Set greater than zero if you want to view less recoreds */
int kMaxRecordCount=0;


int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        if(kPrintError) std::cerr << "No arguments specified!" << std::endl;
        if(kPrintError) print_help(std::cout);
        return 1;
    }
    if(argc < 5)
    {
        if(kPrintError) std::cerr << "Too few arguments specified!" << std::endl;
        if(kPrintError) print_help(std::cout);
        return 1;
    }

    const unsigned int cycle_to_align = 25;

    std::cout << "Cycle Simulator " << INTEROP_VERSION << std::endl;
    std::cout << "Max number of cycles: " << argv[3] << std::endl;
    std::string run_folder = argv[1];
    std::string output_folder = io::combine(argv[2], io::basename(run_folder) + "_MaxCycle_" + argv[3]);
    unsigned int max_cycle = util::lexical_cast<unsigned int>(argv[3]);
    unsigned int max_read = util::lexical_cast<unsigned int>(argv[4]);

    // Somewhat system independent way to create a directory
    std::string cmd = "mkdir "+output_folder;
    int sres = std::system(cmd.c_str());
    if(sres != 0) std::cerr << "Make directory failed for output_folder" << std::endl;
    cmd = "mkdir "+io::combine(output_folder, "InterOp");
    sres = std::system(cmd.c_str());
    if(sres != 0) std::cerr << "Make directory failed for InterOp" << std::endl;

    {
        std::ifstream src(io::combine(run_folder, "RunInfo.xml").c_str(), std::ios::binary);
        std::ofstream dst(io::combine(output_folder, "RunInfo.xml").c_str(), std::ios::binary);
        dst << src.rdbuf();
    }
    {
        std::ifstream src(io::combine(run_folder, "RunParameters.xml").c_str(), std::ios::binary);
        std::ofstream dst(io::combine(output_folder, "RunParameters.xml").c_str(), std::ios::binary);
        dst << src.rdbuf();
    }

    int res = write_interops(run_folder, output_folder, max_cycle, max_read, cycle_to_align);
    if(res != SUCCESS)
    {
        std::cout << "# Error: " << res << std::endl;
        std::cout << "# Version: " << INTEROP_VERSION << std::endl;
        return res;
    }
    return SUCCESS;
}

/** Read metric from a file
 *
 * This function turns expected exceptions into return codes
 *
 * @param filename path to run folder
 * @param metrics metric set
 * @return error code or 0
 */
template<class MetricSet>
int read_metrics_from_file(const std::string& filename, MetricSet& metrics)
{
    try {
        io::read_interop(filename, metrics);
    }
    catch(const io::file_not_found_exception&){return 1;}
    catch(const io::bad_format_exception& ex)
    {
        if(kPrintError) std::cerr << ex.what() << std::endl;
        return BAD_FORMAT;
    }
    catch(const io::incomplete_file_exception&){ }
    catch(const std::exception& ex)
    {
        if(kPrintError) std::cerr << ex.what() << std::endl;
        return UNEXPECTED_EXCEPTION;
    }
    if(metrics.size()==0)
    {
        if(kPrintError) std::cerr << "Empty metric file: " << metrics.name() << std::endl;
        return EMPTY_INTEROP;
    }
    return 0;
}

/** Copy only records less than or equal to max_read
 *
 * @param input path to run folder
 * @param output path to output run folder
 * @param max_read maximum number of reads
 * @return 0 if success, or an error code
 */
int copy_tile_metrics(const std::string& input, const std::string& output, unsigned int max_read)
{
    typedef tile_metrics::metric_array_t metric_array_t;
    typedef metric_array_t::const_iterator const_iterator;
    typedef tile_metric::read_metric_vector read_metric_vector;
    typedef read_metric_vector::const_iterator const_read_iterator;
    tile_metrics metrics;
    int res;
    if((res=read_metrics_from_file(input, metrics)) != 0) return res;

    std::cout << metrics.name() << ": " << metrics.version() << std::endl;
    try {
        io::write_interop(output, metrics);
    }
    catch(const io::bad_format_exception& ex)
    {
        if(kPrintError) std::cerr << ex.what() << std::endl;
        return BAD_FORMAT;
    }

    metric_array_t subset;

    for(const_iterator beg = metrics.metrics().begin(), end=metrics.metrics().end();beg != end;++beg)
    {
        read_metric_vector reads;
        for(const_read_iterator rbeg = beg->read_metrics().begin(), rend=beg->read_metrics().end();rbeg != rend;++rbeg)
        {
            if(rbeg->read() > max_read) continue;
            reads.push_back(*rbeg);
        }

        subset.push_back(tile_metric(*beg, reads));
    }

    tile_metrics metricsOut(subset, metrics.version(), metrics);
    try {
        io::write_interop(output, metricsOut);
    }
    catch(const io::bad_format_exception& ex)
    {
        if(kPrintError) std::cerr << ex.what() << std::endl;
        return BAD_FORMAT;
    }

    return 0;
}

/** Copy only records less than or equal to max_cycle
 *
 * @param input path to run folder
 * @param output path to output run folder
 * @param max_cycle maximum number of cycles
 * @return 0 if success, or an error code
 */
template<class MetricSet>
int copy_cycle_metrics(const std::string& input, const std::string& output, unsigned int max_cycle)
{
    typedef typename MetricSet::metric_array_t metric_array_t;
    typedef typename metric_array_t::const_iterator const_iterator;
    MetricSet metrics;
    int res;
    if((res=read_metrics_from_file(input, metrics)) != 0) return res;

    std::cout << metrics.name() << ": " << metrics.version() << std::endl;

    metric_array_t subset;

    for(const_iterator beg = metrics.metrics().begin(), end=metrics.metrics().end();beg != end;++beg)
    {
        if(beg->cycle() > max_cycle) continue;
        subset.push_back(*beg);
    }

    MetricSet metricsOut(subset, metrics.version(), metrics);
    try {
        io::write_interop(output, metricsOut);
    }
    catch(const io::bad_format_exception& ex)
    {
        if(kPrintError) std::cerr << ex.what() << std::endl;
        return BAD_FORMAT;
    }

    return 0;
}

/** Encode error type and metric type into a single code
 *
 * @param res type of the error
 * @param type type of the metric
 * @return combined code
 */
int encode_error(int res, int type)
{
    return res*100 + type;
}
/** Encode error type and metric type into a single code
 *
 * @param input path to run folder
 * @param output path to output run folder
 * @param max_cycle maximum number of cycles
 * @param max_read maximum number of reads
 * @return 0 if success, or an error code
 */
int write_interops(const std::string& filename, const std::string& output, unsigned int max_cycle, unsigned int max_read, unsigned int cycle_to_align)
{
    int res;
    int valid_count = 0;
    if((res=copy_tile_metrics(filename, output, max_read)) > 1) return encode_error(res, 1);
    if(res == 0) valid_count++;
    if(max_cycle > cycle_to_align)
    {
        if ((res = copy_cycle_metrics<error_metrics>(filename, output, max_cycle)) > 1) return encode_error(res, 2);
        if (res == 0) valid_count++;
    }
    if((res=copy_cycle_metrics<corrected_intensity_metrics>(filename, output, max_cycle)) > 1) return encode_error(res, 2);
    if(res == 0) valid_count++;
    if((res=copy_cycle_metrics<extraction_metrics>(filename, output, max_cycle)) > 1) return encode_error(res, 2);
    if(res == 0) valid_count++;
    if((res=copy_cycle_metrics<q_metrics>(filename, output, max_cycle)) > 1) return encode_error(res, 2);
    if(res == 0) valid_count++;
    if((res=copy_cycle_metrics<image_metrics>(filename, output, max_cycle)) > 1) return encode_error(res, 2);
    if(res == 0) valid_count++;
    if(valid_count == 0)
    {
        if(kPrintError) std::cerr << "No files found" << std::endl;
        return NO_INTEROPS_FOUND;
    }
    return 0;
}

/** Write a help message to the output stream
 *
 * @param out output stream
 */
void print_help(std::ostream& out)
{
    out << "Version: " << INTEROP_VERSION << std::endl;
    out << "Usage: cyclesim run-folder output-folder max-cycle-count max-read-count" << std::endl;
}


