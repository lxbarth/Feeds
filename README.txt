$Id: README.txt,v 1.1 2009/09/16 17:58:12 alexb Exp $


"It feeds"


FEEDS
=====

The new incarnation of FeedAPI and Feed Element Mapper. Work in progress.

Contact alex_b for details:
http://drupal.org/user/53995

Requirements
============

- CTools 1.x-dev or CTools 1.0 with patches
  http://drupal.org/node/599136
  http://drupal.org/node/531522
- Drupal 6.14 or higher.

Installation
============

- Check out from github into your modules directory.
- Navigate to admin/build/feeds.
- Create at least one feed configuration.

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

Todos
=====

- http://drupal.org/project/issues/feeds
- Revisit @todo tags in code
- Option for greying out plugins that miss requirements + message to user.
- Revisit get/set naming conventions.