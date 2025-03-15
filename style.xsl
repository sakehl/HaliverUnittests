<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="html" indent="yes"/>
  <xsl:template match="/">
    <html>
      <head>
        <title>Experiment Results</title>
        <style>
          table {
            width: 100%;
            border-collapse: collapse;
          }
          th, td {
            border: 1px solid black;
            padding: 8px;
            text-align: left;
          }
          th {
            background-color: #f2f2f2;
          }
          .hidden {
            display: none;
          }
        </style>
        <script>
          function toggleVisibility(id) {
            var element = document.getElementById(id);
            if (element.style.display === "none") {
              element.style.display = "block";
            } else {
              element.style.display = "none";
            }
          }
        </script>
      </head>
      <body>
        <h2>Experiment Results</h2>
        <xsl:for-each select="results/group">
          <h3>
            <button onclick="toggleVisibility('group_{position()}')">
              <xsl:value-of select="start_time"/>
              <div><xsl:value-of select="tags"/></div>
            </button>
            
          </h3>
          
          <div id="group_{position()}" class="hidden">
            <table>
              <tr>
                <th>File Name</th>
                <th>Return Code</th>
                <th>Elapsed Time</th>
                <th>Stdout</th>
                <th>Stderr</th>
              </tr>
              <xsl:for-each select="file">
                <tr>
                  <td><xsl:value-of select="name"/></td>
                  <td><xsl:value-of select="return_code"/></td>
                  <td><xsl:value-of select="elapsed_time"/></td>
                  <td>
                    <button onclick="toggleVisibility('stdout_{position()}')">Toggle Stdout</button>
                    <pre id="stdout_{position()}" class="hidden"><xsl:value-of select="stdout"/></pre>
                  </td>
                  <td>
                    <button onclick="toggleVisibility('stderr_{position()}')">Toggle Stderr</button>
                    <pre id="stderr_{position()}" class="hidden"><xsl:value-of select="stderr"/></pre>
                  </td>
                </tr>
              </xsl:for-each>
            </table>
          </div>
        </xsl:for-each>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>