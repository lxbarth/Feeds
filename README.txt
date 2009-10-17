$Id: README.txt,v 1.1 2009/09/16 17:58:12 alexb Exp $


"It feeds"


FEEDS
=====

The new incarnation of FeedAPI and Feed Element Mapper. Work in progress.

Contact alex_b for details:
http://drupal.org/user/53995

Features
========

- Pluggable import configurations consisting of fetchers (get data) parsers
  (read and transform data) and processors (create content on Drupal).
-- HTTP upload.
-- File upload.
-- CSV, RSS, Atom parsing.
-- Creates nodes or terms.
-- Creates lightweight database records if Data module is installed.
   http://drupal.org/project/data
-- Additional fetchers/parsers or processors can be added by an object oriented
   plugin system.
-- Granular mapping of parsed data to content elements.
- Import configurations can be piggy backed on nodes, thus using nodes as
  importers ("feed as node" approach) or they can be used on a standalone form.
- Unlimited number of import configurations.
- Export import configurations to code.
- Optional libraries module support.

Requirements
============

- CTools 1.x-dev or CTools 1.0 with patches
  http://drupal.org/node/599136
  http://drupal.org/node/531522
- Drupal 6.14 or higher.

Installation
============

- Check out from github into your modules directory, enable module.
- Navigate to admin/build/feeds.
- Create at least one feed configuration.
- To use SimplePie parser, download SimplePie and place simplepie.inc into
  feeds/libraries
  http://simplepie.org/

API Overview
============

@todo, in the meantime:

Start reading code in feeds.module and the Feed class in includes/Feed.inc,
take a look at feeds.plugins.inc to see how plugins are registered.

Performance
===========

Untested. There is support for drupal_queue module in the works.
http://drupal.org/node/599180

Hidden settings
===============

Hidden settings are variables that you can define by adding them to the $conf
array in your settings.php file.


Name:        feeds_importer_class
Default:     'FeedsImporter'
Description: The class to use for importing feeds.

Name:        feeds_source_class
Default:     'FeedsSource'
Description: The class to use for handling feed sources.

Name:        feeds_scheduler_class
Default:     'FeedsScheduler'
Description: The class to use for scheduling feed refreshing.

Name:        feeds_worker_time
Default:     60
Description: Execution time for a queue worker, only effective if used with
             drupal_queue.

Name:        feeds_schedule_num
Default:     10
             200 if drupal_queue is enabled
Description: The number of feeds to refresh on cron time.
             If drupal_queue is enabled, the maximum number of feeds to move to
             queue.

Name:        feeds_use_defaults
Default:     TRUE
Description: Whether or not to use default content type and importer
             configurations. Set to FALSE if you would like to use Feeds without
             any default configurations.

Glossary
========

This glossary is introductory and therefore not ordered alphabetically.

Plugin       A plugin is a swappable handler. It can be either a fetcher, a
             parser or a processor.

             See FeedsPlugin class.

Fetcher      A plugin responsible for downloading, loading or receiving data.

             See FeedsFetcher and extending classes.

Parser       A plugin responsible for bringing fetched data into a digestable
             format for processors.

             See FeedsParser and extending classes.

Processor    A plugin that "does stuff" with data. Usually a processor stores
             data in one or the other form (a node, a user, a simple DB record).

             See FeedsProcessor and extending classes.

Feed         A body of data. Can contain a title and feed items. A feed can
             appear in different forms depending on the import stage: before
             fetching a feed would be the external document to be fetched. After
             fetching it would be the raw data dump handed to the parser. After
             parsing it would be the normalized PHP array that is passed to the
             processor.

             Depending on the import stage, a feed is represented by a
             FeedsFetcherResult or a FeedsParserResult.

             See FeedsFetcherResult class, FeedsParserResult class.

Feed item    A feed is assumed to have an array of equally formed entities: feed
             items. The composition of these items depends on the parser.

Feed source  This is the description of the source of a feed. This can be for
             example a URL. But a feed source can also have other properties
             describing a source. For instance, which field delimiter is being
             used in a CSV file.

             See FeedsSource class.

Importer     An importer contains a specific configuration of one fetcher, one
             parser and one processor. It is used to import a feed. Importers
             can be used through a standalone import form available on
             http://www.example.com/import/ or they can be attached to a content
             type. In the latter case feeds are imported by creating nodes of
             such a content type.

             Importers are configured on admin/build/feeds. They are sometimes
             referred to as "Importer configuration or "Configuration".

             See FeedsImporter class.

Import stage The state of the importing process. The import stages are:
             fetching, parsing and processing.

Feed node    A node that is used for importing feeds. A feed node is of a
             content type that has an Importer attached.
