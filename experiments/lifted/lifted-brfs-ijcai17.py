#! /usr/bin/env python
import os

from fslab.common_setup import FSIssueExperiment, IssueConfig
from common import generate_environment, filter_benchmarks_if_test_run, ALL_ATTRIBUTES, add_standard_experiment_steps

TIME_LIMIT = 600
MEMORY_LIMIT = 4000

SUITE = [
    'barman-opt14-strips',
    'cavediving-14-adl',
    'childsnack-opt14-strips',
    'citycar-opt14-adl',
    'floortile-opt14-strips',
    'ged-opt14-strips',
    'hiking-opt14-strips',
    'maintenance-opt14-adl',
    'openstacks-opt14-strips',
    'parking-opt14-strips',
    'tetris-opt14-strips',
    # 'tidybot-opt14-strips', # Duplicate object name cart
    'transport-opt14-strips',
    'visitall-opt14-strips']


REVISIONS = [
    "42ac4b03"
]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]


def configs():
    csp = '--reachability=none --driver bfs-csp --options "verbose_stats=true"'
    # csp_reach_full = '--reachability=full --driver lsbfws --options "verbose_stats=true,evaluator_t=adaptive,bfws.rs=sim,sim.r_all=true"'
    # csp_reach_vars = '--reachability=vars --driver lsbfws --options "verbose_stats=true,evaluator_t=adaptive,bfws.rs=sim,sim.r_all=true"'
    grounded_mt_no_reach = '--reachability=none --driver bfs --options "verbose_stats=true,successor_generation=match_tree"'

    return [
        IssueConfig("grounded-bfs (MT, no reach)", grounded_mt_no_reach.split()),
        # IssueConfig("bfws-r_all-csp-var-reach", csp_reach_vars.split()),
        IssueConfig("lifted-bfs-csp", csp.split()),
        # IssueConfig("bfws-r_all-csp-full-reach", csp_reach_full.split()),
    ]


def main():
    environment = generate_environment(email="guillem.frances@upf.edu",
                                       memory_limit=MEMORY_LIMIT, time_limit=TIME_LIMIT)
    exp = FSIssueExperiment(
        revisions=REVISIONS,
        configs=configs(),
        environment=environment,
        time_limit=TIME_LIMIT,
        memory_limit=MEMORY_LIMIT
    )

    benchmarks = filter_benchmarks_if_test_run(SUITE)

    attributes = [att for name, att in ALL_ATTRIBUTES.items() if not name.startswith('sdd')
                  and not name.startswith('sim') and name not in ('mem_before_mt', 'mem_before_search')]

    exp.add_suite(os.environ["DOWNWARD_BENCHMARKS"], benchmarks)

    add_standard_experiment_steps(exp, add_parse_step=True)

    exp.add_absolute_report_step(attributes=attributes)
    # exp.add_comparison_table_step()

    exp.run_steps()  # Let's go!


if __name__ == '__main__':
    main()