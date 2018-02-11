#
# Copyright (c) 2014-present, Facebook, Inc.
#
# This source code is licensed under the MIT license found in the
# LICENSE file in the root directory of this source tree.
#

from __future__ import absolute_import
from __future__ import print_function
from __future__ import unicode_literals
from __future__ import division

import click

from openr.cli.commands import decision
from openr.cli.utils.utils import parse_nodes


class DecisionCli(object):
    def __init__(self):
        self.decision.add_command(PathCli().path)
        self.decision.add_command(DecisionAdjCli().adj)
        self.decision.add_command(DecisionPrefixesCli().prefixes)
        self.decision.add_command(DecisionRoutesCli().routes)
        self.decision.add_command(DecisionValidateCli().validate)

    @click.group()
    @click.option('--decision_rep_port', default=None, type=int, help='Decision port')
    @click.pass_context
    def decision(ctx, decision_rep_port):  # noqa: B902
        ''' CLI tool to peek into Decision module. '''

        if decision_rep_port:
            ctx.obj.decision_rep_port = decision_rep_port


class PathCli(object):

    @click.command()
    @click.option('--src', default='', help='source node, '
                  'default will be the current host')
    @click.option('--dst', default='', help='destination node or prefix, '
                  'default will be the current host')
    @click.option('--max-hop', default=256, help='max hop count')
    @click.pass_obj
    def path(cli_opts, src, dst, max_hop):  # noqa: B902
        ''' path from src to dst '''

        decision.PathCmd(cli_opts).run(src, dst, max_hop)


class DecisionRoutesCli(object):

    @click.command()
    @click.option('--nodes', default='',
                  help='Get routes for a list of nodes. Default will get '
                       'host\'s routes. Get routes for all nodes if \'all\' is given.')
    @click.option('--prefixes', '-p', default='', multiple=True,
                  help='Get route for specific IPs or Prefixes.')
    @click.option('--json/--no-json', default=False,
                  help='Dump in JSON format')
    @click.pass_obj
    def routes(cli_opts, nodes, prefixes, json):  # noqa: B902
        ''' Request the routing table from Decision module '''

        nodes = parse_nodes(cli_opts.host, nodes, cli_opts.lm_cmd_port)
        decision.DecisionRoutesCmd(cli_opts).run(nodes, prefixes, json)


class DecisionPrefixesCli(object):

    @click.command()
    @click.option('--nodes', default='',
                  help='Dump prefixes for a list of nodes. Default will dump host\'s '
                       'prefixes. Dump prefixes for all nodes if \'all\' is given.')
    @click.option('--json/--no-json', default=False,
                  help='Dump in JSON format')
    @click.pass_obj
    def prefixes(cli_opts, nodes, json):  # noqa: B902
        ''' show the prefixes from Decision module '''

        nodes = parse_nodes(cli_opts.host, nodes, cli_opts.lm_cmd_port)
        decision.DecisionPrefixesCmd(cli_opts).run(nodes, json)


class DecisionAdjCli(object):

    @click.command()
    @click.option('--nodes', default='',
                  help='Dump adjacencies for a list of nodes. Default will dump '
                       'host\'s adjs. Dump adjs for all nodes if \'all\' is given')
    @click.option('--bidir/--no-bidir', default=True,
                  help='Only bidir adjacencies')
    @click.option('--json/--no-json', default=False,
                  help='Dump in JSON format')
    @click.pass_obj
    def adj(cli_opts, nodes, bidir, json):  # noqa: B902
        ''' dump the link-state adjacencies from Decision module '''

        nodes = parse_nodes(cli_opts.host, nodes, cli_opts.lm_cmd_port)
        decision.DecisionAdjCmd(cli_opts).run(nodes, bidir, json)


class DecisionValidateCli(object):

    @click.command()
    @click.pass_obj
    def validate(cli_opts):  # noqa: B902
        ''' Check all prefix & adj dbs in Decision against that in KvStore '''

        decision.DecisionValidateCmd(cli_opts).run()
