/*
 * sidebar.js
 * ~~~~~~~~~~
 *
 * This script makes the Sphinx sidebar collapsible.
 *
 * .sphinxsidebar contains .sphinxsidebarwrapper.  This script adds
 * in .sphinxsidebar, after .sphinxsidebarwrapper, the #sidebarbutton
 * used to collapse and expand the sidebar.
 *
 * When the sidebar is collapsed the .sphinxsidebarwrapper is hidden
 * and the width of the sidebar and the margin-left of the document
 * are decreased. When the sidebar is expanded the opposite happens.
 * This script saves a per-browser/per-session cookie used to
 * remember the position of the sidebar among the pages.
 * Once the browser is closed the cookie is deleted and the position
 * reset to the default (expanded).
 *
 * :copyright: Copyright 2007-2011 by the Sphinx team, see AUTHORS.
 * :license: BSD, see LICENSE for details.
 *
 * pld changes: reverse right and left, etc; fix a declaration, convey hover-color from css
 */

$(function() {
  // global elements used by the functions.
  // the 'sidebarbutton' element is defined as global after its
  // creation, in the add_sidebar_button function
  var bodywrapper = $('.bodywrapper');
  var sidebar = $('.sphinxsidebar');	// sidebar contains sidebarwrapper
  var sidebarwrapper = $('.sphinxsidebarwrapper');

  // for some reason, the document has no sidebar; do not run into errors
  if (!sidebar.length) return;

  // original margin-left of the bodywrapper and width of the sidebar
  // with the sidebar expanded
  var bw_margin_expanded = bodywrapper.css('margin-right');  // pld: was margin-left; = 300px
  var ssb_width_expanded = sidebar.width();	// 280 ?

  // margin-left of the bodywrapper and width of the sidebar
  // with the sidebar collapsed
  var bw_margin_collapsed = '.8em';
  var ssb_width_collapsed = '.8em';

  // colors used by the current theme
  //var dark_color = $('.related').css('background-color');
  //var light_color = $('.document').css('background-color');
  //debugger;
  var light_color; // = 'green';		// redefined below
  var dark_color; //  = '#407FFF';

  function sidebar_is_collapsed() {
    return sidebarwrapper.is(':not(:visible)');
  }

  function toggle_sidebar() {
    if (sidebar_is_collapsed())
      expand_sidebar();
    else
      collapse_sidebar();
  }

// changed by pld
  function collapse_sidebar() {
    sidebarwrapper.hide();
    sidebar.css('width', ssb_width_collapsed);	
    bodywrapper.css('margin-right', bw_margin_collapsed);
    var sidebarbutton = $('#sidebarbutton');		// added by pld
    sidebarbutton.css({
        'margin-right': '0',
        'height': bodywrapper.height()
    });
    sidebarbutton.find('span').text('«');	// was »
    sidebarbutton.attr('title', _('Expand sidebar'));
    document.cookie = 'sidebar=collapsed';
    //debugger;
  }

// changed by pld: margin-left becomes margin-right
  function expand_sidebar() {
    bodywrapper.css('margin-right', bw_margin_expanded);
    sidebar.css('width', ssb_width_expanded);
    sidebarwrapper.show();
    var sidebarbutton = $('#sidebarbutton');		// added by pld
    sidebarbutton.css({
        'margin-right': ssb_width_expanded-12,	// original is 12
        'height': bodywrapper.height()
    });
    sidebarbutton.find('span').text('»');	// was «
    sidebarbutton.attr('title', _('Collapse sidebar'));
    document.cookie = 'sidebar=expanded';
  }

  function add_sidebar_button() {
    sidebarwrapper.css({
        'float': 'right',		// changed to right
        'margin-left': '0',		// changed to left
        'width': ssb_width_expanded - 28
    });
    // create the button
    sidebar.append(
        '<div id="sidebarbutton"><span>&raquo;</span></div>'
    );
    var sidebarbutton = $('#sidebarbutton');
    light_color = sidebarbutton.css('background-color');
    dark_color  = sidebarbutton.css('border-top-color');	// steal this color attribute to be the hover-color
    // find the height of the viewport to center the '<<' in the page
    var viewport_height;
    if (window.innerHeight)
 	  viewport_height = window.innerHeight;
    else
	  viewport_height = $(window).height();
    sidebarbutton.find('span').css({
        'display': 'block',
        'margin-top': (viewport_height - sidebar.position().top - 20) / 2
    });

    sidebarbutton.click(toggle_sidebar);
    sidebarbutton.attr('title', _('Collapse sidebar'));
    sidebarbutton.css({
        'color': '#FFFFFF',
	'background-color': light_color,
        'border-right': '1px solid ' + light_color,	// changed to right
        'font-size': '1.2em',
        'cursor': 'pointer',
        'height': bodywrapper.height(),
        'padding-top': '1px',
        'margin-right': ssb_width_expanded - 12		// changed to right
    });

    sidebarbutton.hover(
      function () {
          $(this).css('background-color', dark_color);
      },
      function () {
          $(this).css('background-color', light_color);
      }
    );
  
  }

  function set_position_from_cookie() {
    if (!document.cookie)
      return;
    var items = document.cookie.split(';');
    for(var k=0; k<items.length; k++) {
      var key_val = items[k].split('=');
      var key = key_val[0];
      if (key == 'sidebar') {
        var value = key_val[1];
        if ((value == 'collapsed') && (!sidebar_is_collapsed()))
          collapse_sidebar();
        else if ((value == 'expanded') && (sidebar_is_collapsed()))
          expand_sidebar();
      }
    }
  }

  add_sidebar_button();
  var sidebarbutton = $('#sidebarbutton');
  set_position_from_cookie();
});
