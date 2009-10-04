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
   * Replace form buttons on mapping form with check boxes and links.
   * @see function feeds_ui_button_form() in feeds_ui.admin.inc
   */
  $(document).ready(function(){
    // Hide the button and show a link instead.
    $('input.feeds-ui-button').hide();
    $('input.feeds-ui-button').each(function(i) {
      $(this).after(
        '<a href="#" class="feeds-ui-button-submit">' + $(this).val() + '</a>'
      );
    });

    // Attach a click handler to the anchor tag that submits the form.
    $('a.feeds-ui-button-submit').click(function() {
      $(this).parent().parent('form').submit();
    });
  });
};