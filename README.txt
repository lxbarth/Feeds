$Id: README.txt,v 1.1 2009/09/16 17:58:12 alexb Exp $

FEEDS
=====

The new incarnation of FeedAPI. Work in progress.

See http://drupal.org/project/issues/feeds for planned features / outstanding
tasks.

Contact alex_b http://drupal.org/user/53995 for details.

API Overview
============

@todo, in the meantime:

Start reading code in feeds.module and the Feed class in includes/Feed.inc, take
a look at feeds.plugins.inc to see how plugins are registered.

Performance
===========

If you are managing more than ten feeds with this module, use drupal_queue module.

Hidden settings
===============

Hidden settings are variables that you can define by adding them to the $conf
array in your settings.php file.

Name: feed_class
Default: 'Feed'
Description: The class to use for handling feed configurations.

Name: feeds_source_class
Default: 'FeedsSource'
Description: The class to use for handling feed sources.

Name: feeds_scheduler_class
Default: 'FeedsScheduler'
Description: The class to use for scheduling feed refresh.

Name: feeds_importer_class
Default: 'FeedsImporter'
Description: The class to use for importing a feed.

Name: feeds_worker_time
Default: 60
Execution time for a queue worker, only effective if used with drupal_queue.
