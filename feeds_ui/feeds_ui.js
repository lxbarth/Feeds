// $Id$

Drupal.behaviors.feeds = function() {

  // Export form machine-readable JS
  $('.feed-name:not(.processed)').each(function() {
    $('.feed-name')
      .addClass('processed')
      .after(' <small class="feed-id-suffix">&nbsp;</small>');
    if ($('.feed-id').val() === $('.feed-name').val().toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/_+/g, '_') || $('.feed-id').val() === '') {
      $('.feed-id').parents('.form-item').hide();
      $('.feed-name').keyup(function() {
        var machine = $(this).val().toLowerCase().replace(/[^a-z0-9]+/g, '_').replace(/_+/g, '_');
        if (machine !== '_' && machine !== '') {
          $('.feed-id').val(machine);
          $('.feed-id-suffix').empty().append(' Machine name: ' + machine + ' [').append($('<a href="#">'+ Drupal.t('Edit') +'</a>').click(function() {
            $('.feed-id').parents('.form-item').show();
            $('.feed-id-suffix').hide();
            $('.feed-name').unbind('keyup');
            return false;
          })).append(']');
        }
        else {
          $('.feed-id').val(machine);
          $('.feed-id-suffix').text('');
        }
      });
      $('.feed-name').keyup();
    }
  });

  // Hide text in specific input fields.
  $('.hide-text-on-focus').focus(function() {
    $(this).val('');
  });

  /**
   * Tune checkboxes on mapping forms.
   * @see feeds_ui_mapping_form() in feeds_ui.admin.inc
   */
  // Hide save button.
  $('input.form-submit.feeds-ui-mapping-save').hide();

  // Attach submit behavior to elements with feeds-ui-trigger-submit class.
  $('.feeds-ui-trigger-submit').click(function() {
    // Use click, not form.submit() - submit() would use the wrong submission
    // handler.
    $('input.form-submit.feeds-ui-mapping-save').click();
  });

  // Replace it with a link.
  $('.feeds-ui-checkbox-link:not(.processed)').each(function(i) {
    $(this).addClass('processed').after(
      '<a href="#" class="feeds-ui-trigger-remove">' + $(this).children(' label').text() + '</a>'
    ).hide();
  });

  // Attach slightly different submit behavior to remove links.
  // We are going to check the box and then submit.
  $('.feeds-ui-trigger-remove').click(function() {
    // Use click, not form.submit() - submit() would use the wrong submission
    // handler.
    $(this).prev().children().children().children().attr('checked', 1);
    $('input.form-submit.feeds-ui-mapping-save').click();
  });
};