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

from openr.cli.commands import fib


class FibCli(object):
    def __init__(self):
        self.fib.add_command(FibRoutesCli().routes)
        self.fib.add_command(FibCountersCli().counters)
        self.fib.add_command(FibListRoutesCli().list_routes, name='list')
        self.fib.add_command(FibAddRoutesCli().add_routes, name='add')
        self.fib.add_command(FibDelRoutesCli().del_routes, name='del')
        self.fib.add_command(FibSyncRoutesCli().sync_routes, name='sync')
        self.fib.add_command(FibValidateRoutesCli().validate)
        self.fib.add_command(FibListRoutesLinuxCli().list_routes_linux,
                             name='list-linux')
        self.fib.add_command(FibValidateRoutesLinuxCli().validate_linux,
                                name='validate-linux')

    @click.group()
    @click.option('--fib_rep_port', default=None, type=int, help='Fib rep port')
    @click.option('--fib_agent_port', default=None, type=int,
                  help='Fib thrift server port')
    @click.option('--client-id', default=None, type=int,
                  help='FIB Client ID')
    @click.pass_context
    def fib(ctx, fib_rep_port, fib_agent_port, client_id):  # noqa: B902
        ''' CLI tool to peek into Fib module. '''

        if fib_rep_port:
            ctx.obj.fib_rep_port = fib_rep_port
        if fib_agent_port:
            ctx.obj.fib_agent_port = fib_agent_port
        if client_id:
            ctx.obj.client_id = client_id


class FibRoutesCli(object):

    @click.command()
    @click.option('--prefixes', '-p', default='', multiple=True,
                  help='Get route for specific IPs or Prefixes.')
    @click.option('--json/--no-json', default=False,
                  help='Dump in JSON format')
    @click.pass_obj
    def routes(cli_opts, prefixes, json):  # noqa: B902
        ''' Request routing table of the current host '''

        fib.FibRoutesCmd(cli_opts).run(prefixes, json)


class FibCountersCli(object):

    @click.command()
    @click.pass_obj
    def counters(cli_opts):  # noqa: B902
        ''' Get various counters on fib agent '''

        fib.FibCountersCmd(cli_opts).run()


class FibListRoutesCli(object):

    @click.command()
    @click.option('--prefixes', '-p', default='', multiple=True,
                  help='Get route for specific IPs or Prefixes.')
    @click.pass_obj
    def list_routes(cli_opts, prefixes):  # noqa: B902
        ''' Get and print all the routes on fib agent '''

        fib.FibListRoutesCmd(cli_opts).run(prefixes)


class FibAddRoutesCli(object):

    @click.command()
    @click.argument('prefixes')   # Comma separated list of prefixes
    @click.argument('nexthops')   # Comma separated list of nexthops
    @click.pass_obj
    def add_routes(cli_opts, prefixes, nexthops):  # noqa: B902
        ''' Add new routes in FIB '''

        fib.FibAddRoutesCmd(cli_opts).run(prefixes, nexthops)


class FibDelRoutesCli(object):

    @click.command()
    @click.argument('prefixes')   # Comma separated list of prefixes
    @click.pass_obj
    def del_routes(cli_opts, prefixes):  # noqa: B902
        ''' Delete routes from FIB '''

        fib.FibDelRoutesCmd(cli_opts).run(prefixes)


class FibSyncRoutesCli(object):

    @click.command()
    @click.argument('prefixes')   # Comma separated list of prefixes
    @click.argument('nexthops')   # Comma separated list of nexthops
    @click.pass_obj
    def sync_routes(cli_opts, prefixes, nexthops):  # noqa: B902
        ''' Re-program FIB with specified routes. Delete all old ones '''

        fib.FibSyncRoutesCmd(cli_opts).run(prefixes, nexthops)


class FibValidateRoutesCli(object):

    @click.command()
    @click.pass_obj
    def validate(cli_opts):  # noqa: B902
        ''' Validator to check that all routes as computed by Decision '''

        fib.FibValidateRoutesCmd(cli_opts).run(cli_opts)


class FibListRoutesLinuxCli(object):

    @click.command()
    @click.option('--prefixes', '-p', default='', multiple=True,
                  help='Get route for specific IPs or Prefixes.')
    @click.pass_obj
    def list_routes_linux(cli_opts, prefixes):  # noqa: B902
        ''' List routes from linux kernel routing table '''

        fib.FibListRoutesLinuxCmd(cli_opts).run(prefixes)


class FibValidateRoutesLinuxCli(object):

    @click.command()
    @click.pass_obj
    def validate_linux(cli_opts):  # noqa: B902
        ''' Validate that FIB routes and Kernel routes match '''

        fib.FibValidateRoutesLinuxCmd().run(cli_opts)
