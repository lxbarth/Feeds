$Id: README.txt,v 1.1 2009/09/16 17:58:12 alexb Exp $

FEEDS

Work in progress.

See http://drupal.org/project/issues/feeds for planned features / outstanding tasks.

Contact alex_b http://drupal.org/user/53995 for details.

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

Name: source_class
Default: 'FeedsSource'
Description: The class to use for handling feed sources.